/*
   Copyright (c) 2009-2010, Jack Poulson
   All rights reserved.

   This file is part of Elemental.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
#include "elemental/blas_internal.hpp"
#include "elemental/lapack_internal.hpp"
using namespace std;
using namespace elemental;
using namespace elemental::utilities;
using namespace elemental::wrappers::blas;
using namespace elemental::wrappers::mpi;

template<typename R>
void
elemental::lapack::internal::PanelTridiagLSquare
( DistMatrix<R,MC,MR  >& A,
  DistMatrix<R,MC,MR  >& W,
  DistMatrix<R,MC,Star>& APan_MC_Star, 
  DistMatrix<R,MR,Star>& APan_MR_Star,
  DistMatrix<R,MC,Star>& W_MC_Star,
  DistMatrix<R,MR,Star>& W_MR_Star )
{
    const int panelSize = W.Width();
    const int bottomSize = W.Height()-panelSize;
    
    const Grid& g = A.Grid();

#ifndef RELEASE
    PushCallStack("lapack::internal::PanelTridiagLSquare");
    if( A.Grid() != W.Grid() )
        throw logic_error
        ( "A and W must be distributed over the same grid." );
    if( A.Height() != A.Width() )
        throw logic_error( "A must be square." );
    if( A.Height() != W.Height() )
        throw logic_error( "A and W must be the same height." );
    if( W.Height() < panelSize )
        throw logic_error( "W must be a column panel." );
    if( W.ColAlignment() != A.ColAlignment() || 
        W.RowAlignment() != A.RowAlignment() )
        throw logic_error( "W and A must be aligned." );
#endif
    const int r = g.Height();

    // Find the process holding the our transposed data
    int transposeRank;
    {
        int colAlignment = A.ColAlignment();
        int rowAlignment = A.RowAlignment();
        int colShift = A.ColShift();
        int rowShift = A.RowShift();

        int transposeRow = (colAlignment+rowShift) % r;
        int transposeCol = (rowAlignment+colShift) % r;
        transposeRank = transposeRow + r*transposeCol;
    }
    bool onDiagonal = ( transposeRank == g.VCRank() );

    // Create a distributed matrix for storing the subdiagonal
    DistMatrix<R,MD,Star> e(g);
    e.AlignWithDiag( A, -1 );
    e.ResizeTo( panelSize, 1 );

    // Matrix views 
    DistMatrix<R,MC,MR> 
        ATL(g), ATR(g),  A00(g), a01(g),     A02(g),  ACol(g),  alpha21T(g),
        ABL(g), ABR(g),  a10(g), alpha11(g), a12(g),            a21B(g),
                         A20(g), a21(g),     A22(g),  A20B(g);
    DistMatrix<R,MC,MR> 
        WTL(g), WTR(g),  W00(g), w01(g),     W02(g),  WCol(g),
        WBL(g), WBR(g),  w10(g), omega11(g), w12(g),
                         W20(g), w21(g),     W22(g),  W20B(g), w21Last(g);
    DistMatrix<R,MD,Star> eT(g),  e0(g),
                          eB(g),  epsilon1(g),
                                  e2(g);

    // Temporary distributions
    vector<R> w21LastLocalBuffer(A.Height()/r+1);
    DistMatrix<R,MC,Star> a21_MC_Star(g);
    DistMatrix<R,MC,Star> a21B_MC_Star(g);
    DistMatrix<R,MR,Star> a21_MR_Star(g);
    DistMatrix<R,MC,Star> p21_MC_Star(g);
    DistMatrix<R,MC,Star> p21B_MC_Star(g);
    DistMatrix<R,MR,Star> p21_MR_Star(g);
    DistMatrix<R,MR,Star> q21_MR_Star(g);
    DistMatrix<R,MR,Star> x01B_MR_Star(g);
    DistMatrix<R,MR,Star> y01B_MR_Star(g);
    DistMatrix<R,MC,Star> a21Last_MC_Star(g);
    DistMatrix<R,MR,Star> a21Last_MR_Star(g);
    DistMatrix<R,MC,Star> w21Last_MC_Star(g);
    DistMatrix<R,MR,Star> w21Last_MR_Star(g);

    // Push to the blocksize of 1, then pop at the end of the routine
    PushBlocksizeStack( 1 );

    PartitionDownLeftDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    PartitionDownLeftDiagonal
    ( W, WTL, WTR,
         WBL, WBR, 0 );
    PartitionDown
    ( e, eT,
         eB, 0 );
    bool firstIteration = true;
    R tau = 0;
    R w21LastFirstEntry = 0;
    while( WTL.Width() < panelSize )
    {
        RepartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, /**/ a01,     A02,
         /*************/ /**********************/
               /**/       a10, /**/ alpha11, a12, 
          ABL, /**/ ABR,  A20, /**/ a21,     A22 );
        
        RepartitionDownDiagonal
        ( WTL, /**/ WTR,  W00, /**/ w01,     W02,
         /*************/ /**********************/
               /**/       w10, /**/ omega11, w12,
          WBL, /**/ WBR,  W20, /**/ w21,     W22 );

        RepartitionDown
        ( eT,  e0,
         /**/ /********/
               epsilon1,
          eB,  e2 );

        ACol.View2x1
        ( alpha11,
          a21 );

        WCol.View2x1
        ( omega11,
          w21 );

        // View the portions of A20 and W20 outside of this panel's square
        A20B.View( A, panelSize, 0, bottomSize, A20.Width() );
        W20B.View( W, panelSize, 0, bottomSize, W20.Width() );

        if( !firstIteration )
        {
            a21Last_MC_Star.View
            ( APan_MC_Star, WTL.Height(), WTL.Width()-1, WBL.Height(), 1 );
            a21Last_MR_Star.View
            ( APan_MR_Star, WTL.Height(), WTL.Width()-1, WBL.Height(), 1 );
            w21Last.View
            ( W, WTL.Height(), WTL.Width()-1, WBL.Height(), 1 );
        }
            
        PartitionDown
        ( a21, alpha21T,
               a21B,     1 );

        a21_MC_Star.AlignWith( A22 );
        a21_MR_Star.AlignWith( A22 );
        p21_MC_Star.AlignWith( A22 );
        p21_MR_Star.AlignWith( A22 );
        q21_MR_Star.AlignWith( A22 );
        a21_MC_Star.ResizeTo( a21.Height(), 1 );
        a21_MR_Star.ResizeTo( a21.Height(), 1 );
        p21_MC_Star.ResizeTo( a21.Height(), 1 );
        p21_MR_Star.ResizeTo( a21.Height(), 1 );
        q21_MR_Star.ResizeTo( a21.Height(), 1 );
        x01B_MR_Star.AlignWith( W20B );
        y01B_MR_Star.AlignWith( W20B );
        x01B_MR_Star.ResizeTo( W20B.Width(), 1 );
        y01B_MR_Star.ResizeTo( W20B.Width(), 1 );

        // View the portions of a21[MC,* ] and p21[MC,* ] below the current
        // panel's square
        a21B_MC_Star.View
        ( a21_MC_Star, a21_MC_Star.Height()-bottomSize, 0, bottomSize, 1 );
        p21B_MC_Star.View
        ( p21_MC_Star, p21_MC_Star.Height()-bottomSize, 0, bottomSize, 1 );
        //--------------------------------------------------------------------//
        const bool thisIsMyCol = ( g.MRRank() == alpha11.RowAlignment() );
        if( thisIsMyCol )
        {
            if( !firstIteration )
            {
                // Finish updating the current column with two axpy's
                int AColLocalHeight = ACol.LocalHeight();
                R* AColLocalBuffer = ACol.LocalBuffer();
                const R* a21Last_MC_Star_LocalBuffer = 
                    a21Last_MC_Star.LocalBuffer();
                for( int i=0; i<AColLocalHeight; ++i )
                    AColLocalBuffer[i] -=
                        w21LastLocalBuffer[i] + 
                        a21Last_MC_Star_LocalBuffer[i]*w21LastFirstEntry;
            }
        }
        if( thisIsMyCol )
        {
            // Compute the Householder reflector
            tau = lapack::internal::ColReflector( alpha21T, a21B );
        }
        // Store the subdiagonal value and turn a21 into a proper scaled 
        // reflector by explicitly placing the implicit one in its first entry
        alpha21T.GetDiagonal( epsilon1 );
        alpha21T.Set( 0, 0, (R)1 );

        // If this is the first iteration, have each member of the owning 
        // process column broadcast tau and a21 within its process row. 
        // Otherwise, also add w21 into the broadcast.
        if( firstIteration )
        {
            const int a21LocalHeight = a21.LocalHeight();
            vector<R> rowBroadcastBuffer(a21LocalHeight+1);
            if( thisIsMyCol )
            {
                // Pack the broadcast buffer with a21 and tau
                memcpy
                ( &rowBroadcastBuffer[0], 
                  a21.LocalBuffer(), 
                  a21LocalHeight*sizeof(R) );
                rowBroadcastBuffer[a21LocalHeight] = tau;
            }
            // Broadcast a21 and tau across the process row
            Broadcast
            ( &rowBroadcastBuffer[0], 
              a21LocalHeight+1, a21.RowAlignment(), g.MRComm() );
            // Store a21[MC,* ] into its DistMatrix class and also store a copy
            // for the next iteration
            memcpy
            ( a21_MC_Star.LocalBuffer(), 
              &rowBroadcastBuffer[0],
              a21LocalHeight*sizeof(R) );
            // Store a21[MC,* ] into APan[MC,* ]
            int APan_MC_Star_Offset = APan_MC_Star.LocalHeight()-a21LocalHeight;
            memcpy
            ( APan_MC_Star.LocalBuffer(APan_MC_Star_Offset,0), 
              &rowBroadcastBuffer[0],
              (APan_MC_Star.LocalHeight()-APan_MC_Star_Offset)*sizeof(R) );
            // Store tau
            tau = rowBroadcastBuffer[a21LocalHeight];
            
            // Take advantage of the square process grid in order to form 
            // a21[MR,* ] from a21[MC,* ]
            if( onDiagonal )
            {
                memcpy
                ( a21_MR_Star.LocalBuffer(),
                  a21_MC_Star.LocalBuffer(),
                  a21LocalHeight*sizeof(R) );
            }
            else
            {
                // Pairwise exchange
                int sendSize = A22.LocalHeight();
                int recvSize = A22.LocalWidth();
                SendRecv
                ( a21_MC_Star.LocalBuffer(), sendSize, transposeRank, 0,
                  a21_MR_Star.LocalBuffer(), recvSize, transposeRank, 0, 
                  g.VCComm() );
            }
            // Store a21[MR,* ]
            int APan_MR_Star_Offset = 
                APan_MR_Star.LocalHeight()-a21_MR_Star.LocalHeight();
            memcpy
            ( APan_MR_Star.LocalBuffer(APan_MR_Star_Offset,A00.Width()),
              a21_MR_Star.LocalBuffer(),
              (APan_MR_Star.LocalHeight()-APan_MR_Star_Offset)*sizeof(R) );
        }
        else
        {
            const int a21LocalHeight = a21.LocalHeight();
            const int w21LastLocalHeight = ACol.LocalHeight();
            vector<R> rowBroadcastBuffer(a21LocalHeight+w21LastLocalHeight+1);
            if( thisIsMyCol ) 
            {
                // Pack the broadcast buffer with a21, w21Last, and tau
                memcpy
                ( &rowBroadcastBuffer[0], 
                  a21.LocalBuffer(),
                  a21LocalHeight*sizeof(R) );
                memcpy
                ( &rowBroadcastBuffer[a21LocalHeight], 
                  &w21LastLocalBuffer[0],
                  w21LastLocalHeight*sizeof(R) );
                rowBroadcastBuffer[a21LocalHeight+w21LastLocalHeight] = tau;
            }
            // Broadcast a21, w21Last, and tau across the process row
            Broadcast
            ( &rowBroadcastBuffer[0], 
              a21LocalHeight+w21LastLocalHeight+1, 
              a21.RowAlignment(), g.MRComm() );
            // Store a21[MC,* ] into its DistMatrix class 
            memcpy
            ( a21_MC_Star.LocalBuffer(), 
              &rowBroadcastBuffer[0],
              a21LocalHeight*sizeof(R) );
            // Store a21[MC,* ] into APan[MC,* ]
            int APan_MC_Star_Offset = APan_MC_Star.LocalHeight()-a21LocalHeight;
            memcpy
            ( APan_MC_Star.LocalBuffer(APan_MC_Star_Offset,A00.Width()), 
              &rowBroadcastBuffer[0],
              (APan_MC_Star.LocalHeight()-APan_MC_Star_Offset)*sizeof(R) );
            // Store w21Last[MC,* ] into its DistMatrix class
            w21Last_MC_Star.AlignWith( alpha11 );
            w21Last_MC_Star.ResizeTo( a21.Height()+1, 1 );
            memcpy
            ( w21Last_MC_Star.LocalBuffer(), 
              &rowBroadcastBuffer[a21LocalHeight], 
              w21LastLocalHeight*sizeof(R) );
            // Store the bottom part of w21Last[MC,* ] into WB[MC,* ] and, 
            // if necessary, W21.
            int W_MC_Star_Offset = W_MC_Star.LocalHeight()-w21LastLocalHeight;
            memcpy
            ( W_MC_Star.LocalBuffer(W_MC_Star_Offset,A00.Width()-1),
              &rowBroadcastBuffer[a21LocalHeight],
              (W_MC_Star.LocalHeight()-W_MC_Star_Offset)*sizeof(R) );
            if( g.MRRank() == w21Last.RowAlignment() )
            {
                memcpy
                ( w21Last.LocalBuffer(),
                  &rowBroadcastBuffer[a21LocalHeight],
                  w21LastLocalHeight*sizeof(R) );
            }
            // Store tau
            tau = rowBroadcastBuffer[a21LocalHeight+w21LastLocalHeight];

            // Take advantage of the square process grid in order to quickly
            // form a21[MR,* ] and w21Last[MR,* ] from their [MC,* ] 
            // counterparts
            w21Last_MR_Star.AlignWith( ABR );
            w21Last_MR_Star.ResizeTo( w21Last.Height(), 1 );
            if( onDiagonal )
            {
                memcpy
                ( a21_MR_Star.LocalBuffer(),
                  a21_MC_Star.LocalBuffer(),
                  a21LocalHeight*sizeof(R) );
                memcpy
                ( w21Last_MR_Star.LocalBuffer(),
                  w21Last_MC_Star.LocalBuffer(),
                  w21LastLocalHeight*sizeof(R) );
            }
            else
            {
                int sendSize = A22.LocalHeight()+ABR.LocalHeight();
                int recvSize = A22.LocalWidth()+ABR.LocalWidth();
                vector<R> sendBuffer(sendSize);
                vector<R> recvBuffer(recvSize);

                // Pack the send buffer
                memcpy
                ( &sendBuffer[0],
                 a21_MC_Star.LocalBuffer(),
                 A22.LocalHeight()*sizeof(R) );
                memcpy
                ( &sendBuffer[A22.LocalHeight()],
                  w21Last_MC_Star.LocalBuffer(),
                  ABR.LocalHeight()*sizeof(R) );

                // Pairwise exchange
                SendRecv
                ( &sendBuffer[0], sendSize, transposeRank, 0,
                  &recvBuffer[0], recvSize, transposeRank, 0,
                  g.VCComm() );

                // Unpack the recv buffer
                memcpy
                ( a21_MR_Star.LocalBuffer(),
                  &recvBuffer[0],
                  A22.LocalWidth()*sizeof(R) );
                memcpy
                ( w21Last_MR_Star.LocalBuffer(),
                  &recvBuffer[A22.LocalWidth()],
                  ABR.LocalWidth()*sizeof(R) );
            }

            // Store w21Last[MR,* ]
            int W_MR_Star_Offset = 
                W_MR_Star.LocalHeight()-w21Last_MR_Star.LocalHeight();
            memcpy
            ( W_MR_Star.LocalBuffer(W_MR_Star_Offset,A00.Width()-1),
              w21Last_MR_Star.LocalBuffer(),
              (W_MR_Star.LocalHeight()-W_MR_Star_Offset)*sizeof(R) );
            // Store a21[MR,* ]
            int APan_MR_Star_Offset = 
                APan_MR_Star.LocalHeight()-a21_MR_Star.LocalHeight();
            memcpy
            ( APan_MR_Star.LocalBuffer(APan_MR_Star_Offset,A00.Width()),
              a21_MR_Star.LocalBuffer(),
              (APan_MR_Star.LocalHeight()-APan_MR_Star_Offset)*sizeof(R) );

            // Update the portion of A22 that is in our current panel with 
            // w21Last and a21Last using two gers. We do not need their top 
            // entries. We trash the upper triangle of our panel of A since we 
            // are only doing slightly more work and we can replace it
            // afterwards.
            DistMatrix<R,MC,Star> a21Last_MC_Star_Bottom(g);
            DistMatrix<R,MR,Star> a21Last_MR_Star_Bottom(g);
            DistMatrix<R,MC,Star> w21Last_MC_Star_Bottom(g);
            DistMatrix<R,MR,Star> w21Last_MR_Star_Bottom(g);
            a21Last_MC_Star_Bottom.View
            ( a21Last_MC_Star, 1, 0, a21Last_MC_Star.Height()-1, 1 );
            a21Last_MR_Star_Bottom.View
            ( a21Last_MR_Star, 1, 0, a21Last_MR_Star.Height()-1, 1 );
            w21Last_MC_Star_Bottom.View
            ( w21Last_MC_Star, 1, 0, w21Last_MC_Star.Height()-1, 1 );
            w21Last_MR_Star_Bottom.View
            ( w21Last_MR_Star, 1, 0, w21Last_MR_Star.Height()-1, 1 );
            const R* a21_MC_Star_Buffer = a21Last_MC_Star_Bottom.LocalBuffer();
            const R* a21_MR_Star_Buffer = a21Last_MR_Star_Bottom.LocalBuffer();
            const R* w21_MC_Star_Buffer = w21Last_MC_Star_Bottom.LocalBuffer();
            const R* w21_MR_Star_Buffer = w21Last_MR_Star_Bottom.LocalBuffer();
            R* A22Buffer = A22.LocalBuffer();
            int localHeight = W22.LocalHeight();
            int localWidth = W22.LocalWidth();
            int lDim = A22.LocalLDim();
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
                for( int iLocal=0; iLocal<localHeight; ++iLocal )
                    A22Buffer[iLocal+jLocal*lDim] -=
                        w21_MC_Star_Buffer[iLocal]*a21_MR_Star_Buffer[jLocal] +
                        a21_MC_Star_Buffer[iLocal]*w21_MR_Star_Buffer[jLocal];

            // We are through with the last iteration's w21
            w21Last_MC_Star.FreeAlignments();
            w21Last_MR_Star.FreeAlignments();
        }

        // Form the local portions of (A22 a21) into p21[MC,* ] and q21[MR,* ]:
        //   p21[MC,* ] := tril(A22)[MC,MR] a21[MR,* ]
        //   q21[MR,* ] := tril(A22,-1)'[MR,MC] a21[MC,* ]
        if( A22.ColShift() > A22.RowShift() )
        {
            // We are below the diagonal, so we can multiply without an 
            // offset for tril(A22)[MC,MR] and tril(A22,-1)'[MR,MC]
            if( A22.LocalHeight() != 0 )
            {
                memcpy
                ( p21_MC_Star.LocalBuffer(),
                  a21_MR_Star.LocalBuffer(),
                  A22.LocalHeight()*sizeof(R) );
                Trmv
                ( 'L', 'N', 'N', A22.LocalHeight(), 
                  A22.LocalBuffer(), A22.LocalLDim(), 
                  p21_MC_Star.LocalBuffer(), 1 );
            }
            if( A22.LocalWidth() != 0 )
            {
                // Our local portion of q21[MR,* ] might be one entry longer 
                // than A22.LocalHeight(), so go ahead and set the last entry 
                // to 0.
                R* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
                q21_MR_Star_LocalBuffer[A22.LocalWidth()-1] = 0;
                memcpy
                ( q21_MR_Star_LocalBuffer,
                  a21_MC_Star.LocalBuffer(),
                  A22.LocalHeight()*sizeof(R) );
                Trmv
                ( 'L', 'T', 'N', A22.LocalHeight(),
                  A22.LocalBuffer(), A22.LocalLDim(),
                  q21_MR_Star_LocalBuffer, 1 );
            }
        }
        else if( A22.ColShift() < A22.RowShift() )
        {
            // We are above the diagonal, so we need to use an offset of +1
            // for both tril(A22)[MC,MR] and tril(A22,-1)'[MR,MC]
            const R* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
            const R* a21_MR_Star_LocalBuffer = a21_MR_Star.LocalBuffer();
            const R* A22LocalBuffer = A22.LocalBuffer();
            if( A22.LocalHeight() != 0 )
            {
                // The first entry of p21[MC,* ] will always be zero due to 
                // the forced offset.
                R* p21_MC_Star_LocalBuffer = p21_MC_Star.LocalBuffer();
                p21_MC_Star_LocalBuffer[0] = 0;
                memcpy
                ( &p21_MC_Star_LocalBuffer[1],
                  a21_MR_Star_LocalBuffer,
                  (A22.LocalHeight()-1)*sizeof(R) );
                Trmv
                ( 'L', 'N', 'N', A22.LocalHeight()-1,
                  &A22LocalBuffer[1], A22.LocalLDim(), 
                  &p21_MC_Star_LocalBuffer[1], 1 );
             }
             if( A22.LocalWidth() != 0 )
             {
                // The last entry of q21[MR,* ] must be zero due to the forced
                // offset.
                R* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
                q21_MR_Star_LocalBuffer[A22.LocalWidth()-1] = 0;
                memcpy
                ( q21_MR_Star_LocalBuffer,
                  &a21_MC_Star_LocalBuffer[1],
                  (A22.LocalHeight()-1)*sizeof(R) );
                Trmv
                ( 'L', 'T', 'N', A22.LocalHeight()-1,
                  &A22LocalBuffer[1], A22.LocalLDim(),
                  q21_MR_Star_LocalBuffer, 1 );
            }
        }
        else
        {
            // We are on the diagonal, so we only need an offset of +1 for
            // tril(A22,-1)'[MR,MC]
            if( A22.LocalHeight() != 0 )
            {
                memcpy
                ( p21_MC_Star.LocalBuffer(),
                  a21_MR_Star.LocalBuffer(),
                  A22.LocalHeight()*sizeof(R) );
                Trmv
                ( 'L', 'N', 'N', A22.LocalHeight(), 
                  A22.LocalBuffer(), A22.LocalLDim(), 
                  p21_MC_Star.LocalBuffer(), 1 );
 
                // The last entry of q21[MR,* ] must be zero due to the forced
                // offset.
                const R* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
                const R* A22LocalBuffer = A22.LocalBuffer();
                R* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
                q21_MR_Star_LocalBuffer[A22.LocalWidth()-1] = 0;
                memcpy
                ( q21_MR_Star_LocalBuffer,
                  &a21_MC_Star_LocalBuffer[1],
                  (A22.LocalHeight()-1)*sizeof(R) );
                Trmv
                ( 'L', 'T', 'N', A22.LocalHeight()-1,
                  &A22LocalBuffer[1], A22.LocalLDim(),
                  q21_MR_Star_LocalBuffer, 1 );
            }
        }

        blas::Gemv
        ( Transpose, 
          (R)1, W20B.LockedLocalMatrix(),
                a21B_MC_Star.LockedLocalMatrix(),
          (R)0, x01B_MR_Star.LocalMatrix() );
        blas::Gemv
        ( Transpose, 
          (R)1, A20B.LockedLocalMatrix(),
                a21B_MC_Star.LockedLocalMatrix(),
          (R)0, y01B_MR_Star.LocalMatrix() );
        // Combine the AllReduce column summations of x01B[MR,* ] and 
        // y01B[MR,* ]
        {
            int x01BLocalHeight = x01B_MR_Star.LocalHeight();
            vector<R> colSummationSendBuffer(2*x01BLocalHeight);
            vector<R> colSummationRecvBuffer(2*x01BLocalHeight);
            memcpy
            ( &colSummationSendBuffer[0], 
              x01B_MR_Star.LocalBuffer(), 
              x01BLocalHeight*sizeof(R) );
            memcpy
            ( &colSummationSendBuffer[x01BLocalHeight],
              y01B_MR_Star.LocalBuffer(), 
              x01BLocalHeight*sizeof(R) );
            AllReduce
            ( &colSummationSendBuffer[0], 
              &colSummationRecvBuffer[0],
              2*x01BLocalHeight, MPI_SUM, g.MCComm() );
            memcpy
            ( x01B_MR_Star.LocalBuffer(), 
              &colSummationRecvBuffer[0], 
              x01BLocalHeight*sizeof(R) );
            memcpy
            ( y01B_MR_Star.LocalBuffer(), 
              &colSummationRecvBuffer[x01BLocalHeight], 
              x01BLocalHeight*sizeof(R) );
        }

        blas::Gemv
        ( Normal, 
          (R)-1, A20B.LockedLocalMatrix(),
                 x01B_MR_Star.LockedLocalMatrix(),
          (R)+1, p21B_MC_Star.LocalMatrix() );
        blas::Gemv
        ( Normal, 
          (R)-1, W20B.LockedLocalMatrix(),
                 y01B_MR_Star.LockedLocalMatrix(),
          (R)+1, p21B_MC_Star.LocalMatrix() );

        // Fast transpose the unsummed q21[MR,* ] -> q21[MC,* ], so that
        // it needs to be summed over process rows instead of process 
        // columns. We immediately add it onto p21[MC,* ], which also needs
        // to be summed within process rows.
        if( onDiagonal )
        {
            int a21LocalHeight = a21.LocalHeight();
            R* p21_MC_Star_LocalBuffer = p21_MC_Star.LocalBuffer();
            const R* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
            for( int i=0; i<a21LocalHeight; ++i )
                p21_MC_Star_LocalBuffer[i] += q21_MR_Star_LocalBuffer[i];
        }
        else
        {
            // Pairwise exchange with the transpose process
            int sendSize = A22.LocalWidth();
            int recvSize = A22.LocalHeight();
            vector<R> recvBuffer(recvSize);
            SendRecv
            ( q21_MR_Star.LocalBuffer(), sendSize, transposeRank, 0,
              &recvBuffer[0],            recvSize, transposeRank, 0,
              g.VCComm() );

            // Unpack the recv buffer directly onto p21[MC,* ]
            R* p21_MC_Star_LocalBuffer = p21_MC_Star.LocalBuffer();
            for( int i=0; i<recvSize; ++i )
                p21_MC_Star_LocalBuffer[i] += recvBuffer[i];
        }

        if( W22.Width() > 0 )
        {
            // This is not the last iteration of the panel factorization, 
            // Reduce to one p21[MC,* ] to the next process column.
            int a21LocalHeight = a21.LocalHeight();

            int nextProcessRow = (alpha11.ColAlignment()+1) % r;
            int nextProcessCol = (alpha11.RowAlignment()+1) % r;

            vector<R> reduceToOneRecvBuffer(a21LocalHeight);
            Reduce
            ( p21_MC_Star.LocalBuffer(),
              &reduceToOneRecvBuffer[0],
              a21LocalHeight, MPI_SUM, nextProcessCol, g.MRComm() );
            if( g.MRRank() == nextProcessCol )
            {
                // Finish computing w21. During its computation, ensure that 
                // every process has a copy of the first element of the w21.
                // We know a priori that the first element of a21 is one.
                R myDotProduct = Dot
                    ( a21LocalHeight, &reduceToOneRecvBuffer[0], 1, 
                                      a21_MC_Star.LocalBuffer(), 1 );
                R sendBuffer[2];
                R recvBuffer[2];
                sendBuffer[0] = myDotProduct;
                sendBuffer[1] = ( g.MCRank()==nextProcessRow ? 
                                  reduceToOneRecvBuffer[0] : 0 );
                AllReduce( sendBuffer, recvBuffer, 2, MPI_SUM, g.MCComm() );
                R dotProduct = recvBuffer[0];

                // Set up for the next iteration by filling in the values for:
                // - w21LastLocalBuffer
                // - w21LastFirstEntry
                R scale = 0.5*dotProduct*tau;
                const R* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
                for( int i=0; i<a21LocalHeight; ++i )
                    w21LastLocalBuffer[i] = tau*
                        ( reduceToOneRecvBuffer[i]-
                          scale*a21_MC_Star_LocalBuffer[i] );
                w21LastFirstEntry = tau*( recvBuffer[1]-scale );
            }
        }
        else
        {
            // This is the last iteration, so we just need to form w21[MC,* ]
            // and w21[MR,* ]. 
            int a21LocalHeight = a21.LocalHeight();

            // AllReduce sum p21[MC,* ] over process rows
            vector<R> allReduceRecvBuffer(a21LocalHeight);
            AllReduce
            ( p21_MC_Star.LocalBuffer(), 
              &allReduceRecvBuffer[0],
              a21LocalHeight, MPI_SUM, g.MRComm() );

            // Finish computing w21. During its computation, ensure that 
            // every process has a copy of the first element of the w21.
            // We know a priori that the first element of a21 is one.
            R myDotProduct = Dot
                ( a21LocalHeight, &allReduceRecvBuffer[0],   1, 
                                  a21_MC_Star.LocalBuffer(), 1 );
            R dotProduct;
            AllReduce( &myDotProduct, &dotProduct, 1, MPI_SUM, g.MCComm() );

            // Grab views into W[MC,* ] and W[MR,* ]
            DistMatrix<R,MC,Star> w21_MC_Star(g);
            DistMatrix<R,MR,Star> w21_MR_Star(g);
            w21_MC_Star.View
            ( W_MC_Star, W00.Height()+1, W00.Width(), w21.Height(), 1 );
            w21_MR_Star.View
            ( W_MR_Star, W00.Height()+1, W00.Width(), w21.Height(), 1 );

            // Store w21[MC,* ]
            R scale = 0.5*dotProduct*tau;
            R* w21_MC_Star_LocalBuffer = w21_MC_Star.LocalBuffer();
            const R* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
            for( int i=0; i<a21LocalHeight; ++i )
                w21_MC_Star_LocalBuffer[i] = tau*
                    ( allReduceRecvBuffer[i]-
                      scale*a21_MC_Star_LocalBuffer[i] );

            // Fast transpose w21[MC,* ] -> w21[MR,* ]
            if( onDiagonal )
            {
                memcpy
                ( w21_MR_Star.LocalBuffer(),
                  w21_MC_Star.LocalBuffer(),
                  a21LocalHeight*sizeof(R) );
            }
            else
            {
                // Pairwise exchange with the transpose process
                int sendSize = A22.LocalHeight();
                int recvSize = A22.LocalWidth();
                SendRecv
                ( w21_MC_Star.LocalBuffer(), sendSize, transposeRank, 0,
                  w21_MR_Star.LocalBuffer(), recvSize, transposeRank, 0,
                  g.VCComm() );
            }
        }
        //--------------------------------------------------------------------//
        a21_MC_Star.FreeAlignments();
        a21_MR_Star.FreeAlignments();
        p21_MC_Star.FreeAlignments();
        p21_MR_Star.FreeAlignments();
        q21_MR_Star.FreeAlignments();
        x01B_MR_Star.FreeAlignments();
        y01B_MR_Star.FreeAlignments();

        SlidePartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, a01,     /**/ A02,
               /**/       a10, alpha11, /**/ a12,
         /*************/ /**********************/
          ABL, /**/ ABR,  A20, a21,     /**/ A22 );

        SlidePartitionDownDiagonal
        ( WTL, /**/ WTR,  W00, w01,     /**/ W02,
               /**/       w10, omega11, /**/ w12,
         /*************/ /**********************/
          WBL, /**/ WBR,  W20, w21,     /**/ W22 );
        
        SlidePartitionDown
        ( eT,  e0,
               epsilon1,
         /**/ /********/
          eB,  e2 );
        firstIteration = false;
    }
    PopBlocksizeStack();

    // View the portion of A that e is the subdiagonal of, then place e into it
    DistMatrix<R,MC,MR> expandedATL(g);
    expandedATL.View( A, 0, 0, panelSize+1, panelSize+1 );
    expandedATL.SetDiagonal( e, -1 );
#ifndef RELEASE
    PopCallStack();
#endif
}

#ifndef WITHOUT_COMPLEX
template<typename R>
void
elemental::lapack::internal::PanelTridiagLSquare
( DistMatrix<complex<R>,MC,MR  >& A,
  DistMatrix<complex<R>,MC,MR  >& W,
  DistMatrix<complex<R>,MD,Star>& t,
  DistMatrix<complex<R>,MC,Star>& APan_MC_Star, 
  DistMatrix<complex<R>,MR,Star>& APan_MR_Star,
  DistMatrix<complex<R>,MC,Star>& W_MC_Star,
  DistMatrix<complex<R>,MR,Star>& W_MR_Star )
{
    typedef complex<R> C;

    const int panelSize = W.Width();
    const int bottomSize = W.Height()-panelSize;

    const Grid& g = A.Grid();

#ifndef RELEASE
    PushCallStack("lapack::internal::PanelTridiagLSquare");
    if( A.Grid() != W.Grid() || W.Grid() != t.Grid() )
        throw logic_error
        ("A, W, and t must be distributed over the same grid.");
    if( A.Height() != A.Width() )
        throw logic_error("A must be square.");
    if( A.Height() != W.Height() )
        throw logic_error( "A and W must be the same height.");
    if( W.Height() < panelSize )
        throw logic_error("W must be a column panel.");
    if( W.ColAlignment() != A.ColAlignment() || 
        W.RowAlignment() != A.RowAlignment() )
        throw logic_error("W and A must be aligned.");
    if( t.Height() != W.Width() || t.Width() != 1 )
        throw logic_error
        ("t must be a column vector of the same length as W's width.");
    if( !t.AlignedWithDiag(A,-1) )
        throw logic_error("t is not aligned with A's subdiagonal.");
#endif
    const int r = g.Height();

    // Find the process holding our transposed data
    int transposeRank;
    {
        int colAlignment = A.ColAlignment();
        int rowAlignment = A.RowAlignment();
        int colShift = A.ColShift();
        int rowShift = A.RowShift();

        int transposeRow = (colAlignment+rowShift) % r;
        int transposeCol = (rowAlignment+colShift) % r;
        transposeRank = transposeRow + r*transposeCol;
    }
    bool onDiagonal = ( transposeRank == g.VCRank() );

    // Create a distributed matrix for storing the subdiagonal
    DistMatrix<R,MD,Star> e(g);
    e.AlignWithDiag( A, -1 );
    e.ResizeTo( panelSize, 1 );

    // Matrix views 
    DistMatrix<C,MC,MR> 
        ATL(g), ATR(g),  A00(g), a01(g),     A02(g),  ACol(g),  alpha21T(g),
        ABL(g), ABR(g),  a10(g), alpha11(g), a12(g),            a21B(g),
                         A20(g), a21(g),     A22(g),  A20B(g);
    DistMatrix<C,MC,MR> 
        WTL(g), WTR(g),  W00(g), w01(g),     W02(g),  WCol(g),
        WBL(g), WBR(g),  w10(g), omega11(g), w12(g),
                         W20(g), w21(g),     W22(g),  W20B(g), w21Last(g);
    DistMatrix<R,MD,Star> eT(g),  e0(g),
                          eB(g),  epsilon1(g),
                                  e2(g);
    DistMatrix<C,MD,Star>
        tT(g),  t0(g),
        tB(g),  tau1(g),
                t2(g);

    // Temporary distributions
    vector<C> w21LastLocalBuffer(A.Height()/r+1);
    DistMatrix<C,MC,Star> a21_MC_Star(g);
    DistMatrix<C,MC,Star> a21B_MC_Star(g);
    DistMatrix<C,MR,Star> a21_MR_Star(g);
    DistMatrix<C,MC,Star> p21_MC_Star(g);
    DistMatrix<C,MC,Star> p21B_MC_Star(g);
    DistMatrix<C,MR,Star> p21_MR_Star(g);
    DistMatrix<C,MR,Star> q21_MR_Star(g);
    DistMatrix<C,MR,Star> x01B_MR_Star(g);
    DistMatrix<C,MR,Star> y01B_MR_Star(g);
    DistMatrix<C,MC,Star> a21Last_MC_Star(g);
    DistMatrix<C,MR,Star> a21Last_MR_Star(g);
    DistMatrix<C,MC,Star> w21Last_MC_Star(g);
    DistMatrix<C,MR,Star> w21Last_MR_Star(g);

    // Push to the blocksize of 1, then pop at the end of the routine
    PushBlocksizeStack( 1 );

    PartitionDownLeftDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    PartitionDownLeftDiagonal
    ( W, WTL, WTR,
         WBL, WBR, 0 );
    PartitionDown
    ( e, eT,
         eB, 0 );
    PartitionDown
    ( t, tT,
         tB, 0 );
    bool firstIteration = true;
    C tau = 0;
    C w21LastFirstEntry = 0;
    while( WTL.Width() < panelSize )
    {
        RepartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, /**/ a01,     A02,
         /*************/ /**********************/
               /**/       a10, /**/ alpha11, a12, 
          ABL, /**/ ABR,  A20, /**/ a21,     A22 );
        
        RepartitionDownDiagonal
        ( WTL, /**/ WTR,  W00, /**/ w01,     W02,
         /*************/ /**********************/
               /**/       w10, /**/ omega11, w12,
          WBL, /**/ WBR,  W20, /**/ w21,     W22 );

        RepartitionDown
        ( eT,  e0,
         /**/ /********/
               epsilon1,
          eB,  e2 );

        RepartitionDown
        ( tT,  t0,
         /**/ /****/
               tau1,
          tB,  t2 );

        ACol.View2x1
        ( alpha11,
          a21 );

        WCol.View2x1
        ( omega11,
          w21 );

        // View the portions of A20 and W20 outside of this panel's square
        A20B.View( A, panelSize, 0, bottomSize, A20.Width() );
        W20B.View( W, panelSize, 0, bottomSize, W20.Width() );

        if( !firstIteration )
        {
            a21Last_MC_Star.View
            ( APan_MC_Star, WTL.Height(), WTL.Width()-1, WBL.Height(), 1 );
            a21Last_MR_Star.View
            ( APan_MR_Star, WTL.Height(), WTL.Width()-1, WBL.Height(), 1 );
            w21Last.View
            ( W, WTL.Height(), WTL.Width()-1, WBL.Height(), 1 );
        }
            
        PartitionDown
        ( a21, alpha21T,
               a21B,     1 );

        a21_MC_Star.AlignWith( A22 );
        a21_MR_Star.AlignWith( A22 );
        p21_MC_Star.AlignWith( A22 );
        p21_MR_Star.AlignWith( A22 );
        q21_MR_Star.AlignWith( A22 );
        a21_MC_Star.ResizeTo( a21.Height(), 1 );
        a21_MR_Star.ResizeTo( a21.Height(), 1 );
        p21_MC_Star.ResizeTo( a21.Height(), 1 );
        p21_MR_Star.ResizeTo( a21.Height(), 1 );
        q21_MR_Star.ResizeTo( a21.Height(), 1 );
        x01B_MR_Star.AlignWith( W20B );
        y01B_MR_Star.AlignWith( W20B );
        x01B_MR_Star.ResizeTo( W20B.Width(), 1 );
        y01B_MR_Star.ResizeTo( W20B.Width(), 1 );

        // View the portions of a21[MC,* ] and p21[MC,* ] below the current
        // panel's square
        a21B_MC_Star.View
        ( a21_MC_Star, a21_MC_Star.Height()-bottomSize, 0, bottomSize, 1 );
        p21B_MC_Star.View
        ( p21_MC_Star, p21_MC_Star.Height()-bottomSize, 0, bottomSize, 1 );
        //--------------------------------------------------------------------//
        const bool thisIsMyCol = ( g.MRRank() == alpha11.RowAlignment() );
        if( thisIsMyCol )
        {
            if( !firstIteration )
            {
                // Finish updating the current column with two axpy's
                int AColLocalHeight = ACol.LocalHeight();
                C* AColLocalBuffer = ACol.LocalBuffer();
                const C* a21Last_MC_Star_LocalBuffer = 
                    a21Last_MC_Star.LocalBuffer();
                for( int i=0; i<AColLocalHeight; ++i )
                    AColLocalBuffer[i] -=
                        w21LastLocalBuffer[i] + 
                        a21Last_MC_Star_LocalBuffer[i]*Conj(w21LastFirstEntry);
            }
        }
        if( thisIsMyCol )
        {
            // Compute the Householder reflector
            tau = lapack::internal::ColReflector( alpha21T, a21B );
            if( g.MCRank() == alpha21T.ColAlignment() )
                tau1.SetLocalEntry(0,0,tau);
        }
        // Store the subdiagonal value and turn a21 into a proper scaled 
        // reflector by explicitly placing the implicit one in its first entry.
        alpha21T.GetRealDiagonal( epsilon1 );
        alpha21T.Set( 0, 0, (C)1 );

        // If this is the first iteration, have each member of the owning 
        // process column broadcast tau and a21 within its process row. 
        // Otherwise, also add w21 into the broadcast.
        if( firstIteration )
        {
            const int a21LocalHeight = a21.LocalHeight();
            vector<C> rowBroadcastBuffer(a21LocalHeight+1);
            if( thisIsMyCol )
            {
                // Pack the broadcast buffer with a21 and tau
                memcpy
                ( &rowBroadcastBuffer[0], 
                  a21.LocalBuffer(), 
                  a21LocalHeight*sizeof(C) );
                rowBroadcastBuffer[a21LocalHeight] = tau;
            }
            // Broadcast a21 and tau across the process row
            Broadcast
            ( &rowBroadcastBuffer[0], 
              a21LocalHeight+1, a21.RowAlignment(), g.MRComm() );
            // Store a21[MC,* ] into its DistMatrix class and also store a copy
            // for the next iteration
            memcpy
            ( a21_MC_Star.LocalBuffer(), 
              &rowBroadcastBuffer[0],
              a21LocalHeight*sizeof(C) );
            // Store a21[MC,* ] into APan[MC,* ]
            int APan_MC_Star_Offset = APan_MC_Star.LocalHeight()-a21LocalHeight;
            memcpy
            ( APan_MC_Star.LocalBuffer(APan_MC_Star_Offset,0), 
              &rowBroadcastBuffer[0],
              (APan_MC_Star.LocalHeight()-APan_MC_Star_Offset)*sizeof(C) );
            // Store tau
            tau = rowBroadcastBuffer[a21LocalHeight];
            
            // Take advantage of the square process grid in order to form 
            // a21[MR,* ] from a21[MC,* ]
            if( onDiagonal )
            {
                memcpy
                ( a21_MR_Star.LocalBuffer(),
                  a21_MC_Star.LocalBuffer(),
                  a21LocalHeight*sizeof(C) );
            }
            else
            {
                // Pairwise exchange
                int sendSize = A22.LocalHeight();
                int recvSize = A22.LocalWidth();
                SendRecv
                ( a21_MC_Star.LocalBuffer(), sendSize, transposeRank, 0,
                  a21_MR_Star.LocalBuffer(), recvSize, transposeRank, 0,
                  g.VCComm() );
            }
            // Store a21[MR,* ]
            int APan_MR_Star_Offset = 
                APan_MR_Star.LocalHeight()-a21_MR_Star.LocalHeight();
            memcpy
            ( APan_MR_Star.LocalBuffer(APan_MR_Star_Offset,A00.Width()),
              a21_MR_Star.LocalBuffer(),
              (APan_MR_Star.LocalHeight()-APan_MR_Star_Offset)*sizeof(C) );
        }
        else
        {
            const int a21LocalHeight = a21.LocalHeight();
            const int w21LastLocalHeight = ACol.LocalHeight();
            vector<C> rowBroadcastBuffer(a21LocalHeight+w21LastLocalHeight+1);
            if( thisIsMyCol ) 
            {
                // Pack the broadcast buffer with a21, w21Last, and tau
                memcpy
                ( &rowBroadcastBuffer[0], 
                  a21.LocalBuffer(),
                  a21LocalHeight*sizeof(C) );
                memcpy
                ( &rowBroadcastBuffer[a21LocalHeight], 
                  &w21LastLocalBuffer[0],
                  w21LastLocalHeight*sizeof(C) );
                rowBroadcastBuffer[a21LocalHeight+w21LastLocalHeight] = tau;
            }
            // Broadcast a21, w21Last, and tau across the process row
            Broadcast
            ( &rowBroadcastBuffer[0], 
              a21LocalHeight+w21LastLocalHeight+1, 
              a21.RowAlignment(), g.MRComm() );
            // Store a21[MC,* ] into its DistMatrix class 
            memcpy
            ( a21_MC_Star.LocalBuffer(), 
              &rowBroadcastBuffer[0],
              a21LocalHeight*sizeof(C) );
            // Store a21[MC,* ] into APan[MC,* ]
            int APan_MC_Star_Offset = APan_MC_Star.LocalHeight()-a21LocalHeight;
            memcpy
            ( APan_MC_Star.LocalBuffer(APan_MC_Star_Offset,A00.Width()), 
              &rowBroadcastBuffer[0],
              (APan_MC_Star.LocalHeight()-APan_MC_Star_Offset)*sizeof(C) );
            // Store w21Last[MC,* ] into its DistMatrix class
            w21Last_MC_Star.AlignWith( alpha11 );
            w21Last_MC_Star.ResizeTo( a21.Height()+1, 1 );
            memcpy
            ( w21Last_MC_Star.LocalBuffer(), 
              &rowBroadcastBuffer[a21LocalHeight], 
              w21LastLocalHeight*sizeof(C) );
            // Store the bottom part of w21Last[MC,* ] into WB[MC,* ] and, 
            // if necessary, W21.
            int W_MC_Star_Offset = W_MC_Star.LocalHeight()-w21LastLocalHeight;
            memcpy
            ( W_MC_Star.LocalBuffer(W_MC_Star_Offset,A00.Width()-1),
              &rowBroadcastBuffer[a21LocalHeight],
              (W_MC_Star.LocalHeight()-W_MC_Star_Offset)*sizeof(C) );
            if( g.MRRank() == w21Last.RowAlignment() )
            {
                memcpy
                ( w21Last.LocalBuffer(),
                  &rowBroadcastBuffer[a21LocalHeight],
                  w21LastLocalHeight*sizeof(C) );
            }
            // Store tau
            tau = rowBroadcastBuffer[a21LocalHeight+w21LastLocalHeight];

            // Take advantage of the square process grid in order to quickly
            // form a21[MR,* ] and w21Last[MR,* ] from their [MC,* ] 
            // counterparts
            w21Last_MR_Star.AlignWith( ABR );
            w21Last_MR_Star.ResizeTo( w21Last.Height(), 1 );
            if( onDiagonal )
            {
                memcpy
                ( a21_MR_Star.LocalBuffer(),
                  a21_MC_Star.LocalBuffer(),
                  a21LocalHeight*sizeof(C) );
                memcpy
                ( w21Last_MR_Star.LocalBuffer(),
                  w21Last_MC_Star.LocalBuffer(),
                  w21LastLocalHeight*sizeof(C) );
            }
            else
            {
                int sendSize = A22.LocalHeight()+ABR.LocalHeight();
                int recvSize = A22.LocalWidth()+ABR.LocalWidth();
                vector<C> sendBuffer(sendSize);
                vector<C> recvBuffer(recvSize);

                // Pack the send buffer
                memcpy
                ( &sendBuffer[0],
                 a21_MC_Star.LocalBuffer(),
                 A22.LocalHeight()*sizeof(C) );
                memcpy
                ( &sendBuffer[A22.LocalHeight()],
                  w21Last_MC_Star.LocalBuffer(),
                  ABR.LocalHeight()*sizeof(C) );

                // Pairwise exchange
                SendRecv
                ( &sendBuffer[0], sendSize, transposeRank, 0,
                  &recvBuffer[0], recvSize, transposeRank, 0,
                  g.VCComm() );

                // Unpack the recv buffer
                memcpy
                ( a21_MR_Star.LocalBuffer(),
                  &recvBuffer[0],
                  A22.LocalWidth()*sizeof(C) );
                memcpy
                ( w21Last_MR_Star.LocalBuffer(),
                  &recvBuffer[A22.LocalWidth()],
                  ABR.LocalWidth()*sizeof(C) );
            }

            // Store w21Last[MR,* ]
            int W_MR_Star_Offset = 
                W_MR_Star.LocalHeight()-w21Last_MR_Star.LocalHeight();
            memcpy
            ( W_MR_Star.LocalBuffer(W_MR_Star_Offset,A00.Width()-1),
              w21Last_MR_Star.LocalBuffer(),
              (W_MR_Star.LocalHeight()-W_MR_Star_Offset)*sizeof(C) );
            // Store a21[MR,* ]
            int APan_MR_Star_Offset = 
                APan_MR_Star.LocalHeight()-a21_MR_Star.LocalHeight();
            memcpy
            ( APan_MR_Star.LocalBuffer(APan_MR_Star_Offset,A00.Width()),
              a21_MR_Star.LocalBuffer(),
              (APan_MR_Star.LocalHeight()-APan_MR_Star_Offset)*sizeof(C) );

            // Update the portion of A22 that is in our current panel with 
            // w21Last and a21Last using two gers. We do not need their top 
            // entries. We trash the upper triangle of our panel of A since we 
            // are only doing slightly more work and we can replace it
            // afterwards.
            DistMatrix<C,MC,Star> a21Last_MC_Star_Bottom(g);
            DistMatrix<C,MR,Star> a21Last_MR_Star_Bottom(g);
            DistMatrix<C,MC,Star> w21Last_MC_Star_Bottom(g);
            DistMatrix<C,MR,Star> w21Last_MR_Star_Bottom(g);
            a21Last_MC_Star_Bottom.View
            ( a21Last_MC_Star, 1, 0, a21Last_MC_Star.Height()-1, 1 );
            a21Last_MR_Star_Bottom.View
            ( a21Last_MR_Star, 1, 0, a21Last_MR_Star.Height()-1, 1 );
            w21Last_MC_Star_Bottom.View
            ( w21Last_MC_Star, 1, 0, w21Last_MC_Star.Height()-1, 1 );
            w21Last_MR_Star_Bottom.View
            ( w21Last_MR_Star, 1, 0, w21Last_MR_Star.Height()-1, 1 );
            const C* a21_MC_Star_Buffer = a21Last_MC_Star_Bottom.LocalBuffer();
            const C* a21_MR_Star_Buffer = a21Last_MR_Star_Bottom.LocalBuffer();
            const C* w21_MC_Star_Buffer = w21Last_MC_Star_Bottom.LocalBuffer();
            const C* w21_MR_Star_Buffer = w21Last_MR_Star_Bottom.LocalBuffer();
            C* A22Buffer = A22.LocalBuffer();
            int localHeight = W22.LocalHeight();
            int localWidth = W22.LocalWidth();
            int lDim = A22.LocalLDim();
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
                for( int iLocal=0; iLocal<localHeight; ++iLocal )
                    A22Buffer[iLocal+jLocal*lDim] -=
                        w21_MC_Star_Buffer[iLocal]*
                        Conj(a21_MR_Star_Buffer[jLocal]) +
                        a21_MC_Star_Buffer[iLocal]*
                        Conj(w21_MR_Star_Buffer[jLocal]);

            // We are through with the last iteration's w21
            w21Last_MC_Star.FreeAlignments();
            w21Last_MR_Star.FreeAlignments();
        }

        // Form the local portions of (A22 a21) into p21[MC,* ] and q21[MR,* ]:
        //   p21[MC,* ] := tril(A22)[MC,MR] a21[MR,* ]
        //   q21[MR,* ] := tril(A22,-1)'[MR,MC] a21[MC,* ]
        if( A22.ColShift() > A22.RowShift() )
        {
            // We are below the diagonal, so we can multiply without an 
            // offset for tril(A22)[MC,MR] and tril(A22,-1)'[MR,MC]
            if( A22.LocalHeight() != 0 )
            {
                memcpy
                ( p21_MC_Star.LocalBuffer(),
                  a21_MR_Star.LocalBuffer(),
                  A22.LocalHeight()*sizeof(C) );
                Trmv
                ( 'L', 'N', 'N', A22.LocalHeight(), 
                  A22.LocalBuffer(), A22.LocalLDim(), 
                  p21_MC_Star.LocalBuffer(), 1 );
            }
            if( A22.LocalWidth() != 0 )
            {
                // Our local portion of q21[MR,* ] might be one entry longer 
                // than A22.LocalHeight(), so go ahead and set the last entry 
                // to 0.
                C* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
                q21_MR_Star_LocalBuffer[A22.LocalWidth()-1] = 0;
                memcpy
                ( q21_MR_Star_LocalBuffer,
                  a21_MC_Star.LocalBuffer(),
                  A22.LocalHeight()*sizeof(C) );
                Trmv
                ( 'L', 'C', 'N', A22.LocalHeight(),
                  A22.LocalBuffer(), A22.LocalLDim(),
                  q21_MR_Star_LocalBuffer, 1 );
            }
        }
        else if( A22.ColShift() < A22.RowShift() )
        {
            // We are above the diagonal, so we need to use an offset of +1
            // for both tril(A22)[MC,MR] and tril(A22,-1)'[MR,MC]
            const C* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
            const C* a21_MR_Star_LocalBuffer = a21_MR_Star.LocalBuffer();
            const C* A22LocalBuffer = A22.LocalBuffer();
            if( A22.LocalHeight() != 0 )
            {
                // The first entry of p21[MC,* ] will always be zero due to 
                // the forced offset.
                C* p21_MC_Star_LocalBuffer = p21_MC_Star.LocalBuffer();
                p21_MC_Star_LocalBuffer[0] = 0;
                memcpy
                ( &p21_MC_Star_LocalBuffer[1],
                  a21_MR_Star_LocalBuffer,
                  (A22.LocalHeight()-1)*sizeof(C) );
                Trmv
                ( 'L', 'N', 'N', A22.LocalHeight()-1,
                  &A22LocalBuffer[1], A22.LocalLDim(), 
                  &p21_MC_Star_LocalBuffer[1], 1 );
            }
            if( A22.LocalWidth() != 0 )
            {
                // The last entry of q21[MR,* ] will be zero if the local
                // height and width are equal.
                C* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
                q21_MR_Star_LocalBuffer[A22.LocalWidth()-1] = 0;
                memcpy
                ( q21_MR_Star_LocalBuffer,
                  &a21_MC_Star_LocalBuffer[1],
                  (A22.LocalHeight()-1)*sizeof(C) );
                Trmv
                ( 'L', 'C', 'N', A22.LocalHeight()-1,
                  &A22LocalBuffer[1], A22.LocalLDim(),
                  q21_MR_Star_LocalBuffer, 1 );
            }
        }
        else
        {
            // We are on the diagonal, so we only need an offset of +1 for
            // tril(A22,-1)'[MR,MC]
            if( A22.LocalHeight() != 0 )
            {
                memcpy
                ( p21_MC_Star.LocalBuffer(),
                  a21_MR_Star.LocalBuffer(),
                  A22.LocalHeight()*sizeof(C) );
                Trmv
                ( 'L', 'N', 'N', A22.LocalHeight(), 
                  A22.LocalBuffer(), A22.LocalLDim(), 
                  p21_MC_Star.LocalBuffer(), 1 );

                // The last entry of q21[MR,* ] will be zero if the local 
                // height and width are equal.
                const C* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
                const C* A22LocalBuffer = A22.LocalBuffer();
                C* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
                q21_MR_Star_LocalBuffer[A22.LocalWidth()-1] = 0;
                memcpy
                ( q21_MR_Star_LocalBuffer,
                  &a21_MC_Star_LocalBuffer[1],
                  (A22.LocalHeight()-1)*sizeof(C) );
                Trmv
                ( 'L', 'C', 'N', A22.LocalHeight()-1,
                  &A22LocalBuffer[1], A22.LocalLDim(),
                  q21_MR_Star_LocalBuffer, 1 );
            }
        }

        blas::Gemv
        ( ConjugateTranspose, 
          (C)1, W20B.LockedLocalMatrix(),
                a21B_MC_Star.LockedLocalMatrix(),
          (C)0, x01B_MR_Star.LocalMatrix() );
        blas::Gemv
        ( ConjugateTranspose, 
          (C)1, A20B.LockedLocalMatrix(),
                a21B_MC_Star.LockedLocalMatrix(),
          (C)0, y01B_MR_Star.LocalMatrix() );
        // Combine the AllReduce column summations of x01B[MR,* ] and 
        // y01B[MR,* ]
        {
            int x01BLocalHeight = x01B_MR_Star.LocalHeight();
            vector<C> colSummationSendBuffer(2*x01BLocalHeight);
            vector<C> colSummationRecvBuffer(2*x01BLocalHeight);
            memcpy
            ( &colSummationSendBuffer[0], 
              x01B_MR_Star.LocalBuffer(), 
              x01BLocalHeight*sizeof(C) );
            memcpy
            ( &colSummationSendBuffer[x01BLocalHeight],
              y01B_MR_Star.LocalBuffer(), 
              x01BLocalHeight*sizeof(C) );
            AllReduce
            ( &colSummationSendBuffer[0], 
              &colSummationRecvBuffer[0],
              2*x01BLocalHeight, MPI_SUM, g.MCComm() );
            memcpy
            ( x01B_MR_Star.LocalBuffer(), 
              &colSummationRecvBuffer[0], 
              x01BLocalHeight*sizeof(C) );
            memcpy
            ( y01B_MR_Star.LocalBuffer(), 
              &colSummationRecvBuffer[x01BLocalHeight], 
              x01BLocalHeight*sizeof(C) );
        }

        blas::Gemv
        ( Normal, 
          (C)-1, A20B.LockedLocalMatrix(),
                 x01B_MR_Star.LockedLocalMatrix(),
          (C)+1, p21B_MC_Star.LocalMatrix() );
        blas::Gemv
        ( Normal, 
          (C)-1, W20B.LockedLocalMatrix(),
                 y01B_MR_Star.LockedLocalMatrix(),
          (C)+1, p21B_MC_Star.LocalMatrix() );

        // Fast transpose the unsummed q21[MR,* ] -> q21[MC,* ], so that
        // it needs to be summed over process rows instead of process 
        // columns. We immediately add it onto p21[MC,* ], which also needs
        // to be summed within process rows.
        if( onDiagonal )
        {
            int a21LocalHeight = a21.LocalHeight();
            C* p21_MC_Star_LocalBuffer = p21_MC_Star.LocalBuffer();
            const C* q21_MR_Star_LocalBuffer = q21_MR_Star.LocalBuffer();
            for( int i=0; i<a21LocalHeight; ++i )
                p21_MC_Star_LocalBuffer[i] += q21_MR_Star_LocalBuffer[i];
        }
        else
        {
            // Pairwise exchange with the transpose process
            int sendSize = A22.LocalWidth();
            int recvSize = A22.LocalHeight();
            vector<C> recvBuffer(recvSize);
            SendRecv
            ( q21_MR_Star.LocalBuffer(), sendSize, transposeRank, 0,
              &recvBuffer[0],            recvSize, transposeRank, 0,
              g.VCComm() );

            // Unpack the recv buffer directly onto p21[MC,* ]
            C* p21_MC_Star_LocalBuffer = p21_MC_Star.LocalBuffer();
            for( int i=0; i<recvSize; ++i )
                p21_MC_Star_LocalBuffer[i] += recvBuffer[i];
        }

        if( W22.Width() > 0 )
        {
            // This is not the last iteration of the panel factorization, 
            // Reduce to one p21[MC,* ] to the next process column.
            int a21LocalHeight = a21.LocalHeight();

            int nextProcessRow = (alpha11.ColAlignment()+1) % r;
            int nextProcessCol = (alpha11.RowAlignment()+1) % r;

            vector<C> reduceToOneRecvBuffer(a21LocalHeight);
            Reduce
            ( p21_MC_Star.LocalBuffer(),
              &reduceToOneRecvBuffer[0],
              a21LocalHeight, MPI_SUM, nextProcessCol, g.MRComm() );
            if( g.MRRank() == nextProcessCol )
            {
                // Finish computing w21. During its computation, ensure that 
                // every process has a copy of the first element of the w21.
                // We know a priori that the first element of a21 is one.
                C myDotProduct = Dot
                    ( a21LocalHeight, &reduceToOneRecvBuffer[0], 1,
                                      a21_MC_Star.LocalBuffer(), 1 );
                C sendBuffer[2];
                C recvBuffer[2];
                sendBuffer[0] = myDotProduct;
                sendBuffer[1] = ( g.MCRank()==nextProcessRow ?
                                  reduceToOneRecvBuffer[0] : 0 );
                AllReduce( sendBuffer, recvBuffer, 2, MPI_SUM, g.MCComm() );
                C dotProduct = recvBuffer[0];

                // Set up for the next iteration by filling in the values for:
                // - w21LastLocalBuffer
                // - w21LastFirstEntry
                C scale = static_cast<C>(0.5)*dotProduct*Conj(tau);
                const C* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
                for( int i=0; i<a21LocalHeight; ++i )
                    w21LastLocalBuffer[i] = tau*
                        ( reduceToOneRecvBuffer[i]-
                          scale*a21_MC_Star_LocalBuffer[i] );
                w21LastFirstEntry = tau*( recvBuffer[1]-scale );
            }
        }
        else
        {
            // This is the last iteration, so we just need to form w21[MC,* ]
            // and w21[MR,* ]. 
            int a21LocalHeight = a21.LocalHeight();

            // AllReduce sum p21[MC,* ] over process rows
            vector<C> allReduceRecvBuffer(a21LocalHeight);
            AllReduce
            ( p21_MC_Star.LocalBuffer(),
              &allReduceRecvBuffer[0],
              a21LocalHeight, MPI_SUM, g.MRComm() );

            // Finish computing w21. During its computation, ensure that 
            // every process has a copy of the first element of the w21.
            // We know a priori that the first element of a21 is one.
            C myDotProduct = Dot
                ( a21LocalHeight, &allReduceRecvBuffer[0],   1,
                                  a21_MC_Star.LocalBuffer(), 1 );
            C dotProduct;
            AllReduce( &myDotProduct, &dotProduct, 1, MPI_SUM, g.MCComm() );

            // Grab views into W[MC,* ] and W[MR,* ]
            DistMatrix<C,MC,Star> w21_MC_Star(g);
            DistMatrix<C,MR,Star> w21_MR_Star(g);
            w21_MC_Star.View
            ( W_MC_Star, W00.Height()+1, W00.Width(), w21.Height(), 1 );
            w21_MR_Star.View
            ( W_MR_Star, W00.Height()+1, W00.Width(), w21.Height(), 1 );

            // Store w21[MC,* ]
            C scale = static_cast<C>(0.5)*dotProduct*Conj(tau);
            C* w21_MC_Star_LocalBuffer = w21_MC_Star.LocalBuffer();
            const C* a21_MC_Star_LocalBuffer = a21_MC_Star.LocalBuffer();
            for( int i=0; i<a21LocalHeight; ++i )
                w21_MC_Star_LocalBuffer[i] = tau*
                    ( allReduceRecvBuffer[i]-
                      scale*a21_MC_Star_LocalBuffer[i] );

            // Fast transpose w21[MC,* ] -> w21[MR,* ]
            if( onDiagonal )
            {
                memcpy
                ( w21_MR_Star.LocalBuffer(),
                  w21_MC_Star.LocalBuffer(),
                  a21LocalHeight*sizeof(C) );
            }
            else
            {
                // Pairwise exchange with the transpose process
                int sendSize = A22.LocalHeight();
                int recvSize = A22.LocalWidth();
                SendRecv
                ( w21_MC_Star.LocalBuffer(), sendSize, transposeRank, 0,
                  w21_MR_Star.LocalBuffer(), recvSize, transposeRank, 0,
                  g.VCComm() );
            }
        }
        //--------------------------------------------------------------------//
        a21_MC_Star.FreeAlignments();
        a21_MR_Star.FreeAlignments();
        p21_MC_Star.FreeAlignments();
        p21_MR_Star.FreeAlignments();
        q21_MR_Star.FreeAlignments();
        x01B_MR_Star.FreeAlignments();
        y01B_MR_Star.FreeAlignments();

        SlidePartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, a01,     /**/ A02,
               /**/       a10, alpha11, /**/ a12,
         /*************/ /**********************/
          ABL, /**/ ABR,  A20, a21,     /**/ A22 );

        SlidePartitionDownDiagonal
        ( WTL, /**/ WTR,  W00, w01,     /**/ W02,
               /**/       w10, omega11, /**/ w12,
         /*************/ /**********************/
          WBL, /**/ WBR,  W20, w21,     /**/ W22 );
        
        SlidePartitionDown
        ( eT,  e0,
               epsilon1,
         /**/ /********/
          eB,  e2 );

        SlidePartitionDown
        ( tT,  t0,
               tau1,
         /**/ /****/
          tB,  t2 );
        firstIteration = false;
    }
    PopBlocksizeStack();

    // View the portion of A that e is the subdiagonal of, then place e into it
    DistMatrix<C,MC,MR> expandedATL(g);
    expandedATL.View( A, 0, 0, panelSize+1, panelSize+1 );
    expandedATL.SetRealDiagonal( e, -1 );
#ifndef RELEASE
    PopCallStack();
#endif
}

#endif // WITHOUT_COMPLEX

template void elemental::lapack::internal::PanelTridiagLSquare
( DistMatrix<float,MC,MR  >& A,
  DistMatrix<float,MC,MR  >& W,
  DistMatrix<float,MC,Star>& APan_MC_Star,
  DistMatrix<float,MR,Star>& APan_MR_Star,
  DistMatrix<float,MC,Star>& W_MC_Star,
  DistMatrix<float,MR,Star>& W_MR_Star );

template void elemental::lapack::internal::PanelTridiagLSquare
( DistMatrix<double,MC,MR  >& A,
  DistMatrix<double,MC,MR  >& W,
  DistMatrix<double,MC,Star>& APan_MC_Star,
  DistMatrix<double,MR,Star>& APan_MR_Star,
  DistMatrix<double,MC,Star>& W_MC_Star,
  DistMatrix<double,MR,Star>& W_MR_Star );

#ifndef WITHOUT_COMPLEX
template void elemental::lapack::internal::PanelTridiagLSquare
( DistMatrix<scomplex,MC,MR  >& A,
  DistMatrix<scomplex,MC,MR  >& W,
  DistMatrix<scomplex,MD,Star>& t,
  DistMatrix<scomplex,MC,Star>& APan_MC_Star,
  DistMatrix<scomplex,MR,Star>& APan_MR_Star,
  DistMatrix<scomplex,MC,Star>& W_MC_Star,
  DistMatrix<scomplex,MR,Star>& W_MR_Star );

template void elemental::lapack::internal::PanelTridiagLSquare
( DistMatrix<dcomplex,MC,MR  >& A,
  DistMatrix<dcomplex,MC,MR  >& W,
  DistMatrix<dcomplex,MD,Star>& t,
  DistMatrix<dcomplex,MC,Star>& APan_MC_Star,
  DistMatrix<dcomplex,MR,Star>& APan_MR_Star,
  DistMatrix<dcomplex,MC,Star>& W_MC_Star,
  DistMatrix<dcomplex,MR,Star>& W_MR_Star );
#endif

