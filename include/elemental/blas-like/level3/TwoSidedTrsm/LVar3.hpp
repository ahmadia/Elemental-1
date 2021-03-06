/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef BLAS_TWOSIDEDTRSM_LVAR3_HPP
#define BLAS_TWOSIDEDTRSM_LVAR3_HPP

#include "elemental/blas-like/level1/Axpy.hpp"
#include "elemental/blas-like/level1/MakeHermitian.hpp"
#include "elemental/blas-like/level1/MakeTriangular.hpp"
#include "elemental/blas-like/level3/Gemm.hpp"
#include "elemental/blas-like/level3/Hemm.hpp"
#include "elemental/blas-like/level3/Her2k.hpp"
#include "elemental/blas-like/level3/Trsm.hpp"
#include "elemental/matrices/Zeros.hpp"

namespace elem {
namespace internal {

template<typename F>
inline void
TwoSidedTrsmLVar3( UnitOrNonUnit diag, Matrix<F>& A, const Matrix<F>& L )
{
#ifndef RELEASE
    CallStackEntry entry("internal::TwoSidedTrsmLVar3");
    if( A.Height() != A.Width() )
        throw std::logic_error("A must be square");
    if( L.Height() != L.Width() )
        throw std::logic_error("Triangular matrices must be square");
    if( A.Height() != L.Height() )
        throw std::logic_error("A and L must be the same size");
#endif
    // Matrix views
    Matrix<F>
        ATL, ATR,  A00, A01, A02,
        ABL, ABR,  A10, A11, A12,
                   A20, A21, A22;
    Matrix<F>
        YTL, YTR,  Y00, Y01, Y02,
        YBL, YBR,  Y10, Y11, Y12,
                   Y20, Y21, Y22;
    Matrix<F>
        LTL, LTR,  L00, L01, L02,
        LBL, LBR,  L10, L11, L12,
                   L20, L21, L22;

    // We will use an entire extra matrix as temporary storage. If this is not
    // acceptable, use TwoSidedTrsmLVar4 instead.
    Matrix<F> Y;
    Zeros( Y, A.Height(), A.Width() );

    PartitionDownDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    PartitionDownDiagonal
    ( Y, YTL, YTR,
         YBL, YBR, 0 );
    LockedPartitionDownDiagonal
    ( L, LTL, LTR,
         LBL, LBR, 0 );
    while( ATL.Height() < A.Height() )
    {
        RepartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, /**/ A01, A02,
         /*************/ /******************/
               /**/       A10, /**/ A11, A12,
          ABL, /**/ ABR,  A20, /**/ A21, A22 );

        RepartitionDownDiagonal
        ( YTL, /**/ YTR,  Y00, /**/ Y01, Y02,
         /*************/ /******************/
               /**/       Y10, /**/ Y11, Y12,
          YBL, /**/ YBR,  Y20, /**/ Y21, Y22 );

        LockedRepartitionDownDiagonal
        ( LTL, /**/ LTR,  L00, /**/ L01, L02,
         /*************/ /******************/
               /**/       L10, /**/ L11, L12,
          LBL, /**/ LBR,  L20, /**/ L21, L22 );

        //--------------------------------------------------------------------//
        // A10 := A10 - 1/2 Y10
        Axpy( F(-1)/F(2), Y10, A10 );

        // A11 := A11 - (A10 L10' + L10 A10')
        Her2k( LOWER, NORMAL, F(-1), A10, L10, F(1), A11 );

        // A11 := inv(L11) A11 inv(L11)'
        TwoSidedTrsmLUnb( diag, A11, L11 );

        // A21 := A21 - A20 L10'
        Gemm( NORMAL, ADJOINT, F(-1), A20, L10, F(1), A21 );

        // A21 := A21 inv(L11)'
        Trsm( RIGHT, LOWER, ADJOINT, diag, F(1), L11, A21 );

        // A10 := A10 - 1/2 Y10
        Axpy( F(-1)/F(2), Y10, A10 );

        // A10 := inv(L11) A10
        Trsm( LEFT, LOWER, NORMAL, diag, F(1), L11, A10 );

        // Y20 := Y20 + L21 A10
        Gemm( NORMAL, NORMAL, F(1), L21, A10, F(1), Y20 );

        // Y21 := L21 A11
        Hemm( RIGHT, LOWER, F(1), A11, L21, F(0), Y21 );

        // Y21 := Y21 + L20 A10'
        Gemm( NORMAL, ADJOINT, F(1), L20, A10, F(1), Y21 );
        //--------------------------------------------------------------------//

        SlidePartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, A01, /**/ A02,
               /**/       A10, A11, /**/ A12,
         /*************/ /******************/
          ABL, /**/ ABR,  A20, A21, /**/ A22 );

        SlidePartitionDownDiagonal
        ( YTL, /**/ YTR,  Y00, Y01, /**/ Y02,
               /**/       Y10, Y11, /**/ Y12,
         /*************/ /******************/
          YBL, /**/ YBR,  Y20, Y21, /**/ Y22 );

        SlideLockedPartitionDownDiagonal
        ( LTL, /**/ LTR,  L00, L01, /**/ L02,
               /**/       L10, L11, /**/ L12,
         /**********************************/
          LBL, /**/ LBR,  L20, L21, /**/ L22 );
    }
}

template<typename F>
inline void
TwoSidedTrsmLVar3
( UnitOrNonUnit diag, DistMatrix<F>& A, const DistMatrix<F>& L )
{
#ifndef RELEASE
    CallStackEntry entry("internal::TwoSidedTrsmLVar3");
    if( A.Height() != A.Width() )
        throw std::logic_error("A must be square");
    if( L.Height() != L.Width() )
        throw std::logic_error("Triangular matrices must be square");
    if( A.Height() != L.Height() )
        throw std::logic_error("A and L must be the same size");
#endif
    const Grid& g = A.Grid();

    // Matrix views
    DistMatrix<F>
        ATL(g), ATR(g),  A00(g), A01(g), A02(g),
        ABL(g), ABR(g),  A10(g), A11(g), A12(g),
                         A20(g), A21(g), A22(g);
    DistMatrix<F>
        YTL(g), YTR(g),  Y00(g), Y01(g), Y02(g),
        YBL(g), YBR(g),  Y10(g), Y11(g), Y12(g),
                         Y20(g), Y21(g), Y22(g);
    DistMatrix<F>
        LTL(g), LTR(g),  L00(g), L01(g), L02(g),
        LBL(g), LBR(g),  L10(g), L11(g), L12(g),
                         L20(g), L21(g), L22(g);

    // Temporary distributions
    DistMatrix<F,STAR,MR  > A11_STAR_MR(g);
    DistMatrix<F,STAR,STAR> A11_STAR_STAR(g);
    DistMatrix<F,VC,  STAR> A21_VC_STAR(g);
    DistMatrix<F,STAR,VR  > A10_STAR_VR(g);
    DistMatrix<F,STAR,MR  > A10_STAR_MR(g);
    DistMatrix<F,STAR,STAR> L11_STAR_STAR(g);
    DistMatrix<F,STAR,VR  > L10_STAR_VR(g);
    DistMatrix<F,STAR,MR  > L10_STAR_MR(g);
    DistMatrix<F,MC,  STAR> L21_MC_STAR(g);
    DistMatrix<F,STAR,STAR> X11_STAR_STAR(g);
    DistMatrix<F,MC,  STAR> X21_MC_STAR(g);
    DistMatrix<F,MC,  STAR> Z21_MC_STAR(g);

    // We will use an entire extra matrix as temporary storage. If this is not
    // acceptable, use TwoSidedTrsmLVar4 instead.
    DistMatrix<F> Y(g);
    Y.AlignWith( A );
    Zeros( Y, A.Height(), A.Width() );

    PartitionDownDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    PartitionDownDiagonal
    ( Y, YTL, YTR,
         YBL, YBR, 0 );
    LockedPartitionDownDiagonal
    ( L, LTL, LTR,
         LBL, LBR, 0 );
    while( ATL.Height() < A.Height() )
    {
        RepartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, /**/ A01, A02,
         /*************/ /******************/
               /**/       A10, /**/ A11, A12,
          ABL, /**/ ABR,  A20, /**/ A21, A22 );

        RepartitionDownDiagonal
        ( YTL, /**/ YTR,  Y00, /**/ Y01, Y02,
         /*************/ /******************/
               /**/       Y10, /**/ Y11, Y12,
          YBL, /**/ YBR,  Y20, /**/ Y21, Y22 );

        LockedRepartitionDownDiagonal
        ( LTL, /**/ LTR,  L00, /**/ L01, L02,
         /*************/ /******************/
               /**/       L10, /**/ L11, L12,
          LBL, /**/ LBR,  L20, /**/ L21, L22 );

        A11_STAR_MR.AlignWith( Y21 );
        A21_VC_STAR.AlignWith( A21 );
        A10_STAR_VR.AlignWith( A10 );
        A10_STAR_MR.AlignWith( A10 );
        L10_STAR_VR.AlignWith( A10 );
        L10_STAR_MR.AlignWith( A10 );
        L21_MC_STAR.AlignWith( Y21 );
        X21_MC_STAR.AlignWith( A20 );
        Z21_MC_STAR.AlignWith( L20 );
        //--------------------------------------------------------------------//
        // A10 := A10 - 1/2 Y10
        Axpy( F(-1)/F(2), Y10, A10 );

        // A11 := A11 - (A10 L10' + L10 A10')
        A10_STAR_VR = A10;
        L10_STAR_VR = L10;
        Zeros( X11_STAR_STAR, A11.Height(), A11.Width() );
        Her2k
        ( LOWER, NORMAL, 
          F(1), A10_STAR_VR.Matrix(), L10_STAR_VR.Matrix(),
          F(0), X11_STAR_STAR.Matrix() );
        MakeTriangular( LOWER, X11_STAR_STAR );
        A11.SumScatterUpdate( F(-1), X11_STAR_STAR );

        // A11 := inv(L11) A11 inv(L11)'
        A11_STAR_STAR = A11;
        L11_STAR_STAR = L11;
        LocalTwoSidedTrsm( LOWER, diag, A11_STAR_STAR, L11_STAR_STAR );
        A11 = A11_STAR_STAR;

        // A21 := A21 - A20 L10'
        L10_STAR_MR = L10_STAR_VR;
        LocalGemm( NORMAL, ADJOINT, F(1), A20, L10_STAR_MR, X21_MC_STAR );
        A21.SumScatterUpdate( F(-1), X21_MC_STAR );

        // A21 := A21 inv(L11)'
        A21_VC_STAR = A21;
        LocalTrsm
        ( RIGHT, LOWER, ADJOINT, diag, F(1), L11_STAR_STAR, A21_VC_STAR );
        A21 = A21_VC_STAR;

        // A10 := A10 - 1/2 Y10
        Axpy( F(-1)/F(2), Y10, A10 );

        // A10 := inv(L11) A10
        A10_STAR_VR = A10;
        LocalTrsm
        ( LEFT, LOWER, NORMAL, diag, F(1), L11_STAR_STAR, A10_STAR_VR );

        // Y20 := Y20 + L21 A10
        A10_STAR_MR = A10_STAR_VR;
        A10 = A10_STAR_MR;
        L21_MC_STAR = L21;
        LocalGemm( NORMAL, NORMAL, F(1), L21_MC_STAR, A10_STAR_MR, F(1), Y20 );

        // Y21 := L21 A11
        MakeHermitian( LOWER, A11_STAR_STAR );
        A11_STAR_MR = A11_STAR_STAR;
        LocalGemm( NORMAL, NORMAL, F(1), L21_MC_STAR, A11_STAR_MR, F(0), Y21 );

        // Y21 := Y21 + L20 A10'
        LocalGemm( NORMAL, ADJOINT, F(1), L20, A10_STAR_MR, Z21_MC_STAR );
        Y21.SumScatterUpdate( F(1), Z21_MC_STAR );
        //--------------------------------------------------------------------//
        A11_STAR_MR.FreeAlignments();
        A21_VC_STAR.FreeAlignments();
        A10_STAR_VR.FreeAlignments();
        A10_STAR_MR.FreeAlignments();
        L10_STAR_VR.FreeAlignments();
        L10_STAR_MR.FreeAlignments();
        L21_MC_STAR.FreeAlignments();
        X21_MC_STAR.FreeAlignments();
        Z21_MC_STAR.FreeAlignments();

        SlidePartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, A01, /**/ A02,
               /**/       A10, A11, /**/ A12,
         /*************/ /******************/
          ABL, /**/ ABR,  A20, A21, /**/ A22 );

        SlidePartitionDownDiagonal
        ( YTL, /**/ YTR,  Y00, Y01, /**/ Y02,
               /**/       Y10, Y11, /**/ Y12,
         /*************/ /******************/
          YBL, /**/ YBR,  Y20, Y21, /**/ Y22 );

        SlideLockedPartitionDownDiagonal
        ( LTL, /**/ LTR,  L00, L01, /**/ L02,
               /**/       L10, L11, /**/ L12,
         /**********************************/
          LBL, /**/ LBR,  L20, L21, /**/ L22 );
    }
}

} // namespace internal
} // namespace elem

#endif // ifndef BLAS_TWOSIDEDTRSM_LVAR3_HPP
