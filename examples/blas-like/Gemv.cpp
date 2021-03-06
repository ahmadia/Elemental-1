/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
// NOTE: It is possible to simply include "elemental.hpp" instead
#include "elemental-lite.hpp"
#include "elemental/blas-like/level2/Gemv.hpp"
#include "elemental/matrices/Uniform.hpp"
using namespace std;
using namespace elem;

// Typedef our real and complex types to 'R' and 'C' for convenience
typedef double R;
typedef Complex<R> C;

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
        const bool adjoint = Input("--adjoint","apply adjoint?",false);
        const bool print = Input("--print","print matrices?",false);
        ProcessInput();
        PrintInputReport();

        const Orientation orientation = ( adjoint ? ADJOINT : NORMAL );

        Grid g( comm );
        DistMatrix<C> A( g );
        Uniform( A, m, n );

        // Draw the entries of the original x and y from uniform distributions 
        // over the complex unit ball
        DistMatrix<C,VC,STAR> x( g ), y( g );
        if( orientation == NORMAL )
        {
            Uniform( x, n, 1 );
            Uniform( y, m, 1 );
        }
        else
        {
            Uniform( x, m, 1 );
            Uniform( y, n, 1 );
        }

        if( print )
        {
            A.Print("A");
            x.Print("x");
            y.Print("y");
        }

        // Run the matrix-vector product
        Gemv( orientation, C(3), A, x, C(4), y );

        if( print )
        {
            if( orientation == NORMAL )
                y.Print("y := 3 A x + 4 y");
            else
                y.Print("y := 3 A^H x + 4 y");
        }
    }
    catch( ArgException& e )
    {
        // There is nothing to do
    }
    catch( exception& e )
    {
        std::ostringstream os;
        os << "Process " << commRank << " caught exception with message: "
           << e.what() << endl;
        std::cerr << os.str();
#ifndef RELEASE
        DumpCallStack();
#endif
    }

    Finalize();
    return 0;
}

