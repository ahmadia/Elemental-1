/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef MATRICES_PEI_HPP
#define MATRICES_PEI_HPP

namespace elem {

template<typename T> 
inline void
Pei( Matrix<T>& P, int n, T alpha )
{
#ifndef RELEASE
    CallStackEntry entry("Pei");
#endif
    P.ResizeTo( n, n );
    for( int j=0; j<n; ++j )
        for( int i=0; i<n; ++i )
            P.Set( i, j, T(1) );
    for( int j=0; j<n; ++j )
        P.Update( j, j, alpha );
}

template<typename T,Distribution U,Distribution V>
inline void
Pei( DistMatrix<T,U,V>& P, int n, T alpha )
{
#ifndef RELEASE
    CallStackEntry entry("MakeIdentity");
#endif
    P.ResizeTo( n, n );
    const int localHeight = P.LocalHeight();
    const int localWidth = P.LocalWidth();
    const int colShift = P.ColShift();
    const int rowShift = P.RowShift();
    const int colStride = P.ColStride();
    const int rowStride = P.RowStride();
    for( int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const int j = rowShift + jLocal*rowStride;
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
        {
            const int i = colShift + iLocal*colStride;
            P.SetLocal( iLocal, jLocal, T(1) );
            if( i == j )
                P.UpdateLocal( iLocal, jLocal, alpha );
        }
    }
}

} // namespace elem

#endif // ifndef MATRICES_PEI_HPP
