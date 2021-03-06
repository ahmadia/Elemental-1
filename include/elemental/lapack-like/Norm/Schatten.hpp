/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef LAPACK_NORM_SCHATTEN_HPP
#define LAPACK_NORM_SCHATTEN_HPP

#include "elemental/blas-like/level1/MakeHermitian.hpp"
#include "elemental/blas-like/level1/MakeSymmetric.hpp"
#include "elemental/lapack-like/SVD.hpp"

namespace elem {

template<typename F> 
inline BASE(F)
SchattenNorm( const Matrix<F>& A, BASE(F) p )
{
#ifndef RELEASE
    CallStackEntry entry("SchattenNorm");
#endif
    typedef BASE(F) R;
    Matrix<F> B( A );
    Matrix<R> s;
    SVD( B, s );

    // TODO: Think of how to make this more stable
    const int k = s.Height();
    R sum = 0;
    for( int j=k-1; j>=0; --j )
        sum += Pow( s.Get(j,0), p ); 
    return Pow( sum, 1/p ); 
}

template<typename F>
inline BASE(F)
HermitianSchattenNorm
( UpperOrLower uplo, const Matrix<F>& A, BASE(F) p )
{
#ifndef RELEASE
    CallStackEntry entry("HermitianSchattenNorm");
#endif
    typedef BASE(F) R;

    Matrix<F> B( A );
    Matrix<R> s;
    HermitianSVD( uplo, B, s );

    // TODO: Think of how to make this more stable
    const int k = s.Height();
    R sum = 0;
    for( int j=k-1; j>=0; --j )
        sum += Pow( s.Get(j,0), p );
    return Pow( sum, 1/p );
}

template<typename F>
inline BASE(F)
SymmetricSchattenNorm
( UpperOrLower uplo, const Matrix<F>& A, BASE(F) p )
{
#ifndef RELEASE
    CallStackEntry entry("SymmetricSchattenNorm");
#endif
    typedef BASE(F) R;
    Matrix<F> B( A );
    Matrix<R> s;
    MakeSymmetric( uplo, B );
    SVD( B, s );

    // TODO: Think of how to make this more stable
    const int k = s.Height();
    R sum = 0;
    for( int j=0; j<k; ++j )
        sum += Pow( s.Get(j,0), p );
    return Pow( sum, 1/p );
}

template<typename F,Distribution U,Distribution V> 
inline BASE(F)
SchattenNorm( const DistMatrix<F,U,V>& A, BASE(F) p )
{
#ifndef RELEASE
    CallStackEntry entry("SchattenNorm");
#endif
    typedef BASE(F) R;
    DistMatrix<F> B( A );
    DistMatrix<R,VR,STAR> s( A.Grid() );
    SVD( B, s );

    // TODO: Think of how to make this more stable
    const int kLocal = s.LocalHeight();
    R localSum = 0;
    for( int j=kLocal-1; j>=0; --j ) 
        localSum += Pow( s.GetLocal(j,0), p );
    R sum;
    mpi::AllReduce( &localSum, &sum, 1, mpi::SUM, A.Grid().VRComm() );
    return Pow( sum, 1/p );
}

template<typename F,Distribution U,Distribution V>
inline BASE(F)
HermitianSchattenNorm
( UpperOrLower uplo, const DistMatrix<F,U,V>& A, BASE(F) p )
{
#ifndef RELEASE
    CallStackEntry entry("HermitianSchattenNorm");
#endif
    typedef BASE(F) R;

    DistMatrix<F> B( A );
    DistMatrix<R,VR,STAR> s( A.Grid() );
    HermitianSVD( uplo, B, s );

    // TODO: Think of how to make this more stable
    const int kLocal = s.LocalHeight();
    R localSum = 0;
    for( int j=kLocal-1; j>=0; --j )
        localSum += Pow( s.GetLocal(j,0), p );
    R sum;
    mpi::AllReduce( &localSum, &sum, 1, mpi::SUM, A.Grid().VRComm() );
    return Pow( sum, 1/p );
}

template<typename F,Distribution U,Distribution V>
inline BASE(F)
SymmetricSchattenNorm
( UpperOrLower uplo, const DistMatrix<F,U,V>& A, BASE(F) p )
{
#ifndef RELEASE
    CallStackEntry entry("SymmetricSchattenNorm");
#endif
    typedef BASE(F) R;
    DistMatrix<F> B( A );
    DistMatrix<R,VR,STAR> s( A.Grid() );
    MakeSymmetric( uplo, B );
    SVD( B, s );

    // TODO: Think of how to make this more stable
    const int kLocal = s.LocalHeight();
    R localSum = 0;
    for( int j=0; j<kLocal; ++j )
        localSum += Pow( s.GetLocal(j,0), p );
    R sum;
    mpi::AllReduce( &localSum, &sum, 1, mpi::SUM, A.Grid().VRComm() );
    return Pow( sum, 1/p );
}

} // namespace elem

#endif // ifndef LAPACK_NORM_SCHATTEN_HPP
