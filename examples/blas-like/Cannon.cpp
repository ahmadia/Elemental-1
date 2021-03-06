/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
// NOTE: It is possible to simply include "elemental.hpp" instead
#include "elemental-lite.hpp"
#include "elemental/blas-like/level3/Gemm.hpp"
#include "elemental/matrices/Uniform.hpp"
using namespace std;
using namespace elem;

int
main( int argc, char* argv[] )
{
    Initialize( argc, argv );

    mpi::Comm comm = mpi::COMM_WORLD;
    const int commRank = mpi::CommRank( comm );
    try 
    {
        const int m = Input("--height","height of C",100);
        const int n = Input("--width","width of C",100);
        const int k = Input("--inner","inner dimension",100);
        const double alpha = Input("--alpha","scale of A B",2.);
        const double beta = Input("--beta","scale of C",3.);
        const bool print = Input("--print","print matrices?",false);
        ProcessInput();
        PrintInputReport();

        Grid g( comm );
        if( g.Height() != g.Width() )
            throw std::logic_error("This routine requires a square grid");

        DistMatrix<double> A(g), B(g), C(g);
        Uniform( A, m, k );
        Uniform( B, k, n );
        Uniform( C, m, n );
        if( print )
        {
            A.Print("A");
            B.Print("B");
            C.Print("C");
        }

        gemm::Cannon_NN( alpha, A, B, beta, C );
        if( print )
            C.Print("C := alpha A B + beta C");
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

