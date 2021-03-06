/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef BLAS_TRSM_RUT_HPP
#define BLAS_TRSM_RUT_HPP

#include "elemental/blas-like/level3/Gemm.hpp"

namespace elem {
namespace internal {

// Right Upper (Conjugate)Transpose (Non)Unit Trsm
//   X := X triu(U)^-T, 
//   X := X triu(U)^-H,
//   X := X triuu(U)^-T, or
//   X := X triuu(U)^-H
template<typename F>
inline void
TrsmRUT
( Orientation orientation, UnitOrNonUnit diag,
  F alpha, const DistMatrix<F>& U, DistMatrix<F>& X,
  bool checkIfSingular )
{
#ifndef RELEASE
    CallStackEntry entry("internal::TrsmRUT");
    if( orientation == NORMAL )
        throw std::logic_error("TrsmRUT expects a (Conjugate)Transpose option");
#endif
    const Grid& g = U.Grid();

    // Matrix views
    DistMatrix<F> 
        UTL(g), UTR(g),  U00(g), U01(g), U02(g),
        UBL(g), UBR(g),  U10(g), U11(g), U12(g),
                         U20(g), U21(g), U22(g);

    DistMatrix<F> XL(g), XR(g),
                  X0(g), X1(g), X2(g);

    // Temporary distributions
    DistMatrix<F,VR,  STAR> U01_VR_STAR(g);
    DistMatrix<F,STAR,MR  > U01AdjOrTrans_STAR_MR(g);
    DistMatrix<F,STAR,STAR> U11_STAR_STAR(g);
    DistMatrix<F,VC,  STAR> X1_VC_STAR(g);
    DistMatrix<F,STAR,MC  > X1Trans_STAR_MC(g);
    
    // Start the algorithm
    Scale( alpha, X );
    LockedPartitionUpDiagonal
    ( U, UTL, UTR,
         UBL, UBR, 0 );
    PartitionLeft( X, XL, XR, 0 );
    while( XL.Width() > 0 )
    {
        LockedRepartitionUpDiagonal
        ( UTL, /**/ UTR,  U00, U01, /**/ U02,
               /**/       U10, U11, /**/ U12,
         /*************/ /******************/
          UBL, /**/ UBR,  U20, U21, /**/ U22 );

        RepartitionLeft
        ( XL,     /**/ XR,
          X0, X1, /**/ X2 );

        X1_VC_STAR.AlignWith( X0 );
        X1Trans_STAR_MC.AlignWith( X0 );
        U01_VR_STAR.AlignWith( X0 );
        U01AdjOrTrans_STAR_MR.AlignWith( X0 );
        //--------------------------------------------------------------------//
        U11_STAR_STAR = U11;
        X1_VC_STAR = X1; 

        LocalTrsm
        ( RIGHT, UPPER, orientation, diag, 
          F(1), U11_STAR_STAR, X1_VC_STAR, checkIfSingular );

        X1Trans_STAR_MC.TransposeFrom( X1_VC_STAR );
        X1.TransposeFrom( X1Trans_STAR_MC );
        U01_VR_STAR = U01;
        if( orientation == ADJOINT )
            U01AdjOrTrans_STAR_MR.AdjointFrom( U01_VR_STAR );
        else
            U01AdjOrTrans_STAR_MR.TransposeFrom( U01_VR_STAR );

        // X0[MC,MR] -= X1[MC,* ] (U01[MR,* ])^(T/H)
        //            = X1^T[* ,MC] (U01^(T/H))[* ,MR]
        LocalGemm
        ( TRANSPOSE, NORMAL, 
          F(-1), X1Trans_STAR_MC, U01AdjOrTrans_STAR_MR, F(1), X0 );
        //--------------------------------------------------------------------//
        X1_VC_STAR.FreeAlignments();
        X1Trans_STAR_MC.FreeAlignments();
        U01_VR_STAR.FreeAlignments();
        U01AdjOrTrans_STAR_MR.FreeAlignments();

        SlideLockedPartitionUpDiagonal
        ( UTL, /**/ UTR,  U00, /**/ U01, U02,
         /*************/ /******************/
               /**/       U10, /**/ U11, U12, 
          UBL, /**/ UBR,  U20, /**/ U21, U22 );

        SlidePartitionLeft
        ( XL, /**/     XR,
          X0, /**/ X1, X2 );
    }
}

} // namespace internal
} // namespace elem

#endif // ifndef BLAS_TRSM_RUT_HPP
