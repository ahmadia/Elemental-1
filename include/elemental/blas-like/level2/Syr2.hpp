/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef BLAS_SYR2_HPP
#define BLAS_SYR2_HPP

namespace elem {

template<typename T>
inline void
Syr2
( UpperOrLower uplo,
  T alpha, const Matrix<T>& x, const Matrix<T>& y, Matrix<T>& A, 
  bool conjugate=false )
{
#ifndef RELEASE
    CallStackEntry entry("Syr2");
    if( A.Height() != A.Width() )
        throw std::logic_error("A must be square");
    if( (x.Width() != 1 && x.Height() != 1) ||
        (y.Width() != 1 && y.Height() != 1) )
        throw std::logic_error("x and y must be vectors");
    const int xLength = ( x.Width()==1 ? x.Height() : x.Width() );
    const int yLength = ( y.Width()==1 ? y.Height() : y.Width() );
    if( xLength != A.Height() || yLength != A.Height() )
        throw std::logic_error("x and y must conform with A");
#endif
    const char uploChar = UpperOrLowerToChar( uplo );
    const int m = A.Height();
    const int incx = ( x.Width()==1 ? 1 : x.LDim() );
    const int incy = ( y.Width()==1 ? 1 : y.LDim() );
    if( conjugate )
    {
        blas::Her2
        ( uploChar, m,
          alpha, x.LockedBuffer(), incx, y.LockedBuffer(), incy,
                 A.Buffer(), A.LDim() );
    }
    else
    {
        blas::Syr2
        ( uploChar, m,
          alpha, x.LockedBuffer(), incx, y.LockedBuffer(), incy,
                 A.Buffer(), A.LDim() );
    }
}

template<typename T>
inline void
Syr2
( UpperOrLower uplo,
  T alpha, const DistMatrix<T>& x,
           const DistMatrix<T>& y,
                 DistMatrix<T>& A,
  bool conjugate=false )
{
#ifndef RELEASE
    CallStackEntry entry("Syr2");
    if( A.Grid() != x.Grid() || x.Grid() != y.Grid() )
        throw std::logic_error
        ("{A,x,y} must be distributed over the same grid");
    if( A.Height() != A.Width() )
        throw std::logic_error("A must be square");
    const int xLength = ( x.Width()==1 ? x.Height() : x.Width() );
    const int yLength = ( y.Width()==1 ? y.Height() : y.Width() );
    if( A.Height() != xLength || A.Height() != yLength )
    {
        std::ostringstream msg;
        msg << "A must conform with x: \n"
            << "  A ~ " << A.Height() << " x " << A.Width() << "\n"
            << "  x ~ " << x.Height() << " x " << x.Width() << "\n"
            << "  y ~ " << y.Height() << " x " << y.Width() << "\n";
        throw std::logic_error( msg.str() );
    }
#endif
    const Grid& g = A.Grid();

    const int localHeight = A.LocalHeight();
    const int localWidth = A.LocalWidth();
    const int r = g.Height();
    const int c = g.Width();
    const int colShift = A.ColShift();
    const int rowShift = A.RowShift();

    if( x.Width() == 1 && y.Width() == 1 )
    {
        DistMatrix<T,MC,STAR> x_MC_STAR(g), y_MC_STAR(g);
        DistMatrix<T,MR,STAR> x_MR_STAR(g), y_MR_STAR(g);

        x_MC_STAR.AlignWith( A );
        x_MR_STAR.AlignWith( A );
        y_MC_STAR.AlignWith( A );
        y_MR_STAR.AlignWith( A );
        //--------------------------------------------------------------------//
        x_MC_STAR = x;
        x_MR_STAR = x_MC_STAR;
        y_MC_STAR = y;
        y_MR_STAR = y_MC_STAR;

        const T* xBuffer = x_MC_STAR.LockedBuffer();
        const T* yBuffer = y_MC_STAR.LockedBuffer();
        if( uplo == LOWER )
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightAboveDiag = Length(j,colShift,r);

                const T beta = y_MR_STAR.GetLocal(jLocal,0);
                const T kappa = x_MR_STAR.GetLocal(jLocal,0);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=heightAboveDiag; iLocal<localHeight; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal] +
                                    delta*yBuffer[iLocal];
            }
        }
        else
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightToDiag = Length(j+1,colShift,r);

                const T beta = y_MR_STAR.GetLocal(jLocal,0);
                const T kappa = x_MR_STAR.GetLocal(jLocal,0);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=0; iLocal<heightToDiag; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal] + 
                                    delta*yBuffer[iLocal];
            }
        }
        //--------------------------------------------------------------------//
        x_MC_STAR.FreeAlignments();
        x_MR_STAR.FreeAlignments();
        y_MC_STAR.FreeAlignments();
        y_MR_STAR.FreeAlignments();
    }
    else if( x.Width() == 1 )
    {
        DistMatrix<T,MC,STAR> x_MC_STAR(g);
        DistMatrix<T,MR,STAR> x_MR_STAR(g);
        DistMatrix<T,STAR,MC> y_STAR_MC(g);
        DistMatrix<T,STAR,MR> y_STAR_MR(g);

        x_MC_STAR.AlignWith( A );
        x_MR_STAR.AlignWith( A );
        y_STAR_MC.AlignWith( A );
        y_STAR_MR.AlignWith( A );
        //--------------------------------------------------------------------//
        x_MC_STAR = x;
        x_MR_STAR = x_MC_STAR;
        y_STAR_MR = y;
        y_STAR_MC = y_STAR_MR;

        const T* xBuffer = x_MC_STAR.LockedBuffer();
        const T* yBuffer = y_STAR_MC.LockedBuffer();
        const int incy = y_STAR_MC.LDim();
        if( uplo == LOWER )
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightAboveDiag = Length(j,colShift,r);

                const T beta = y_STAR_MR.GetLocal(0,jLocal);
                const T kappa = x_MR_STAR.GetLocal(jLocal,0);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=heightAboveDiag; iLocal<localHeight; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal] +
                                    delta*yBuffer[iLocal*incy];
            }
        }
        else
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightToDiag = Length(j+1,colShift,r);

                const T beta = y_STAR_MR.GetLocal(0,jLocal);
                const T kappa = x_MR_STAR.GetLocal(jLocal,0);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=0; iLocal<heightToDiag; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal] +
                                    delta*yBuffer[iLocal*incy];
            }
        }
        //--------------------------------------------------------------------//
        x_MC_STAR.FreeAlignments();
        x_MR_STAR.FreeAlignments();
        y_STAR_MC.FreeAlignments();
        y_STAR_MR.FreeAlignments();
    }
    else if( y.Width() == 1 )
    {
        DistMatrix<T,STAR,MC> x_STAR_MC(g);
        DistMatrix<T,STAR,MR> x_STAR_MR(g);
        DistMatrix<T,MC,STAR> y_MC_STAR(g);
        DistMatrix<T,MR,STAR> y_MR_STAR(g);

        x_STAR_MC.AlignWith( A );
        x_STAR_MR.AlignWith( A );
        y_MC_STAR.AlignWith( A );
        y_MR_STAR.AlignWith( A );
        //--------------------------------------------------------------------//
        x_STAR_MR = x;
        x_STAR_MC = x_STAR_MR;
        y_MC_STAR = y;
        y_MR_STAR = y_MC_STAR;

        const T* xBuffer = x_STAR_MC.LockedBuffer();
        const T* yBuffer = y_MC_STAR.LockedBuffer();
        const int incx = x_STAR_MC.LDim();
        if( uplo == LOWER )
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightAboveDiag = Length(j,colShift,r);

                const T beta = x_STAR_MR.GetLocal(0,jLocal);
                const T kappa = y_MR_STAR.GetLocal(jLocal,0);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=heightAboveDiag; iLocal<localHeight; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal*incx] +
                                    delta*yBuffer[iLocal]; 
            }
        }
        else
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightToDiag = Length(j+1,colShift,r);

                const T beta = x_STAR_MR.GetLocal(0,jLocal);
                const T kappa = y_MR_STAR.GetLocal(jLocal,0);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=0; iLocal<heightToDiag; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal*incx] +
                                    delta*yBuffer[iLocal];
            }
        }
        //--------------------------------------------------------------------//
        x_STAR_MC.FreeAlignments();
        x_STAR_MR.FreeAlignments();
        y_MC_STAR.FreeAlignments();
        y_MR_STAR.FreeAlignments();
    }
    else
    {
        DistMatrix<T,STAR,MC> x_STAR_MC(g), y_STAR_MC(g);
        DistMatrix<T,STAR,MR> x_STAR_MR(g), y_STAR_MR(g);

        x_STAR_MC.AlignWith( A );
        x_STAR_MR.AlignWith( A );
        y_STAR_MC.AlignWith( A );
        y_STAR_MR.AlignWith( A );
        //--------------------------------------------------------------------//
        x_STAR_MR = x;
        x_STAR_MC = x_STAR_MR;
        y_STAR_MR = y;
        y_STAR_MC = y_STAR_MR;

        const T* xBuffer = x_STAR_MC.LockedBuffer();
        const T* yBuffer = y_STAR_MC.LockedBuffer();
        const int incx = x_STAR_MC.LDim();
        const int incy = y_STAR_MC.LDim();
        if( uplo == LOWER )
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightAboveDiag = Length(j,colShift,r);

                const T beta = y_STAR_MR.GetLocal(0,jLocal);
                const T kappa = x_STAR_MR.GetLocal(0,jLocal);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=heightAboveDiag; iLocal<localHeight; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal*incx] +
                                    delta*yBuffer[iLocal*incy];
            }
        }
        else
        {
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const int j = rowShift + jLocal*c;
                const int heightToDiag = Length(j+1,colShift,r);

                const T beta = y_STAR_MR.GetLocal(0,jLocal);
                const T kappa = x_STAR_MR.GetLocal(0,jLocal);
                const T gamma = ( conjugate ? alpha*Conj(beta) : alpha*beta );
                const T delta = ( conjugate ? alpha*Conj(kappa) : alpha*kappa );
                T* ACol = A.Buffer(0,jLocal);
                for( int iLocal=0; iLocal<heightToDiag; ++iLocal )
                    ACol[iLocal] += gamma*xBuffer[iLocal*incx] +
                                    delta*yBuffer[iLocal*incy];
            }
        }
        //--------------------------------------------------------------------//
        x_STAR_MC.FreeAlignments();
        x_STAR_MR.FreeAlignments();
        y_STAR_MC.FreeAlignments();
        y_STAR_MR.FreeAlignments();
    }
}

} // namespace elem

#endif // ifndef BLAS_SYR2_HPP
