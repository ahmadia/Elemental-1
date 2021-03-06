/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
// NOTE: It is possible to simply include "elemental.hpp" instead
#include "elemental-lite.hpp"
#include "elemental/blas-like/level1/MakeTriangular.hpp"
#include "elemental/lapack-like/ApplyColumnPivots.hpp"
#include "elemental/lapack-like/ApplyPackedReflectors.hpp"
#include "elemental/lapack-like/QR.hpp"
#include "elemental/lapack-like/Norm/Frobenius.hpp"
#include "elemental/matrices/Identity.hpp"
#include "elemental/matrices/Uniform.hpp"
using namespace std;
using namespace elem;

typedef double Real;
typedef Complex<Real> C;

int
main( int argc, char* argv[] )
{
    Initialize( argc, argv );

    mpi::Comm comm = mpi::COMM_WORLD;
    const int commRank = mpi::CommRank( comm );

    try 
    {
        const int m = Input("--height","height of matrix",100);
        const int n = Input("--width","width of matrix",100);
        const bool alwaysRecompute = Input("--always","no norm updates?",false);
        const bool blockedUnpiv = 
            Input("--blockUnpiv","blocked unpivoted QR?",false);
        const bool print = Input("--print","print matrices?",false);
        ProcessInput();
        PrintInputReport();

        DistMatrix<C> A;
        Uniform( A, m, n );
        const Real frobA = FrobeniusNorm( A );
        if( print )
            A.Print("A");

        // Compute the pivoted QR decomposition of A, but do not overwrite A
        DistMatrix<C> QRPiv( A );
        DistMatrix<C,MD,STAR> tPiv;
        DistMatrix<int,VR,STAR> p;
        qr::BusingerGolub( QRPiv, tPiv, p, alwaysRecompute );
        if( print )
        {
            QRPiv.Print("QRPiv");
            tPiv.Print("tPiv");
            p.Print("p");
        }

        // Compute the standard QR decomposition of A
        DistMatrix<C> QRNoPiv( A );
        DistMatrix<C,MD,STAR> tNoPiv;
        if( blockedUnpiv )
            QR( QRNoPiv, tNoPiv );
        else
            qr::PanelHouseholder( QRNoPiv, tNoPiv );
        if( print )
        {
            QRNoPiv.Print("QRNoPiv");
            tNoPiv.Print("tNoPiv");
        }

        // Check the error in the pivoted QR factorization, 
        // || A P - Q R ||_F / || A ||_F
        DistMatrix<C> E( QRPiv );
        MakeTriangular( UPPER, E );
        ApplyPackedReflectors
        ( LEFT, LOWER, VERTICAL, BACKWARD, UNCONJUGATED, 0, QRPiv, tPiv, E );
        ApplyInverseColumnPivots( E, p ); 
        Axpy( C(-1), A, E );
        const Real frobQRPiv = FrobeniusNorm( E );
        if( print )
            E.Print("A P - Q R");

        // Check the error in the standard QR factorization, 
        // || A - Q R ||_F / || A ||_F
        E = QRNoPiv;
        MakeTriangular( UPPER, E );
        ApplyPackedReflectors
        ( LEFT, LOWER, VERTICAL, BACKWARD, UNCONJUGATED, 0, 
          QRNoPiv, tNoPiv, E );
        Axpy( C(-1), A, E );
        const Real frobQRNoPiv = FrobeniusNorm( E );
        if( print )
            E.Print("A - Q R");

        // Check orthogonality of pivoted Q, || I - Q^H Q ||_F / || A ||_F
        Identity( E, m, n );
        ApplyPackedReflectors
        ( LEFT, LOWER, VERTICAL, BACKWARD, UNCONJUGATED, 0, QRPiv, tPiv, E );
        ApplyPackedReflectors
        ( LEFT, LOWER, VERTICAL, FORWARD, CONJUGATED, 0, QRPiv, tPiv, E );
        const int k = std::min(m,n);
        DistMatrix<C> EUpper;
        View( EUpper, E, 0, 0, k, k );
        DistMatrix<C> I;
        Identity( I, k, k );
        Axpy( C(-1), I, EUpper );
        const Real frobOrthogPiv = FrobeniusNorm( EUpper ); 
        if( print )
            E.Print("pivoted I - Q^H Q");

        // Check orthogonality of unpivoted Q, || I - Q^H Q ||_F / || A ||_F
        Identity( E, m, n );
        ApplyPackedReflectors
        ( LEFT, LOWER, VERTICAL, BACKWARD, UNCONJUGATED, 0, 
          QRNoPiv, tNoPiv, E );
        ApplyPackedReflectors
        ( LEFT, LOWER, VERTICAL, FORWARD, CONJUGATED, 0, 
          QRNoPiv, tNoPiv, E );
        View( EUpper, E, 0, 0, k, k );
        Identity( I, k, k );
        Axpy( C(-1), I, EUpper );
        const Real frobOrthogNoPiv = FrobeniusNorm( EUpper ); 
        if( print )
            E.Print("unpivoted I - Q^H Q");

        if( commRank == 0 )
        {
            std::cout << "|| A ||_F = " << frobA << "\n\n"
                      << "With pivoting: \n" 
                      << "    || A P - Q R ||_F / || A ||_F = " 
                      << frobQRPiv/frobA << "\n"
                      << "    || I - Q^H Q ||_F / || A ||_F = "
                      << frobOrthogPiv/frobA << "\n\n"
                      << "Without pivoting: \n"
                      << "    || A - Q R ||_F / || A ||_F = "
                      << frobQRNoPiv/frobA << "\n"
                      << "    || I - Q^H Q ||_F / || A ||_F = "
                      << frobOrthogNoPiv/frobA << "\n"
                      << std::endl;
        }
    }
    catch( ArgException& e )
    {
        // There is nothing to do
    }
    catch( exception& e )
    {
        ostringstream os;
        os << "Process " << commRank << " caught exception with message: "
           << e.what() << endl;
        cerr << os.str();
#ifndef RELEASE
        DumpCallStack();
#endif
    }

    Finalize();
    return 0;
}
