/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef BLAS_HERK_HPP
#define BLAS_HERK_HPP

#include "elemental/blas-like/level3/Syrk.hpp"

namespace elem {

template<typename T>
inline void
Herk
( UpperOrLower uplo, Orientation orientation,
  T alpha, const Matrix<T>& A, T beta, Matrix<T>& C )
{
#ifndef RELEASE
    CallStackEntry entry("Herk");
#endif
    Syrk( uplo, orientation, alpha, A, beta, C, true );
}

template<typename T>
inline void
Herk
( UpperOrLower uplo, Orientation orientation,
  T alpha, const Matrix<T>& A, Matrix<T>& C )
{
#ifndef RELEASE
    CallStackEntry entry("Herk");
#endif
    const int n = ( orientation==NORMAL ? A.Height() : A.Width() );
    Zeros( C, n, n );
    Syrk( uplo, orientation, alpha, A, T(0), C, true );
}

template<typename T>
inline void
Herk
( UpperOrLower uplo, Orientation orientation,
  T alpha, const DistMatrix<T>& A, T beta, DistMatrix<T>& C )
{
#ifndef RELEASE
    CallStackEntry entry("Herk");
#endif
    Syrk( uplo, orientation, alpha, A, beta, C, true );
}

template<typename T>
inline void
Herk
( UpperOrLower uplo, Orientation orientation,
  T alpha, const DistMatrix<T>& A, DistMatrix<T>& C )
{
#ifndef RELEASE
    CallStackEntry entry("Herk");
#endif
    const int n = ( orientation==NORMAL ? A.Height() : A.Width() );
    Zeros( C, n, n );
    Syrk( uplo, orientation, alpha, A, T(0), C, true );
}

} // namespace elem

#endif // ifndef BLAS_HERK_HPP
