/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
// NOTE: It is possible to simply include "elemental.hpp" instead
#include "elemental-lite.hpp"
#include "elemental/blas-like/level3/Hemm.hpp"
#include "elemental/blas-like/level3/Trmm.hpp"
#include "elemental/blas-like/level3/TwoSidedTrmm.hpp"
#include "elemental/lapack-like/Norm/Frobenius.hpp"
#include "elemental/lapack-like/Norm/Infinity.hpp"
#include "elemental/lapack-like/Norm/One.hpp"
#include "elemental/matrices/HermitianUniformSpectrum.hpp"
using namespace std;
using namespace elem;

template<typename F> 
void TestCorrectness
( bool print, UpperOrLower uplo, UnitOrNonUnit diag,
  const DistMatrix<F>& A, const DistMatrix<F>& B, const DistMatrix<F>& AOrig )
{
    typedef BASE(F) R;
    const Grid& g = A.Grid();
    const int m = AOrig.Height();

    const int k=100;
    DistMatrix<F> X(g), Y(g), Z(g);
    Uniform( X, m, k );
    Y = X;
    Zeros( Z, m, k );

    if( uplo == LOWER )
    {
        // Test correctness by comparing the application of A against a 
        // random set of k vectors to the application of 
        // tril(B)^H AOrig tril(B)
        Trmm( LEFT, LOWER, NORMAL, diag, F(1), B, Y );
        Hemm( LEFT, LOWER, F(1), AOrig, Y, F(0), Z );
        Trmm( LEFT, LOWER, ADJOINT, diag, F(1), B, Z );
        Hemm( LEFT, LOWER, F(-1), A, X, F(1), Z );
        R infNormOfAOrig = HermitianInfinityNorm( uplo, AOrig );
        R frobNormOfAOrig = HermitianFrobeniusNorm( uplo, AOrig );
        R infNormOfA = HermitianInfinityNorm( uplo, A );
        R frobNormOfA = HermitianFrobeniusNorm( uplo, A );
        R oneNormOfError = OneNorm( Z );
        R infNormOfError = InfinityNorm( Z );
        R frobNormOfError = FrobeniusNorm( Z );
        if( g.Rank() == 0 )
        {
            cout << "||AOrig||_1 = ||AOrig||_oo     = "
                 << infNormOfAOrig << "\n"
                 << "||AOrig||_F                    = "
                 << frobNormOfAOrig << "\n"
                 << "||A||_1 = ||A||_oo             = "
                 << infNormOfA << "\n"
                 << "||A||_F                        = "
                 << frobNormOfA << "\n"
                 << "||A X - L^H AOrig L X||_1  = "
                 << oneNormOfError << "\n"
                 << "||A X - L^H AOrig L X||_oo = " 
                 << infNormOfError << "\n"
                 << "||A X - L^H AOrig L X||_F  = "
                 << frobNormOfError << endl;
        }
    }
    else
    {
        // Test correctness by comparing the application of A against a 
        // random set of k vectors to the application of 
        // triu(B) AOrig triu(B)^H
        Trmm( LEFT, UPPER, ADJOINT, diag, F(1), B, Y );
        Hemm( LEFT, UPPER, F(1), AOrig, Y, F(0), Z );
        Trmm( LEFT, UPPER, NORMAL, diag, F(1), B, Z );
        Hemm( LEFT, UPPER, F(-1), A, X, F(1), Z );
        R infNormOfAOrig = HermitianInfinityNorm( uplo, AOrig );
        R frobNormOfAOrig = HermitianFrobeniusNorm( uplo, AOrig );
        R infNormOfA = HermitianInfinityNorm( uplo, A );
        R frobNormOfA = HermitianFrobeniusNorm( uplo, A );
        R oneNormOfError = OneNorm( Z );
        R infNormOfError = InfinityNorm( Z );
        R frobNormOfError = FrobeniusNorm( Z );
        if( g.Rank() == 0 )
        {
            cout << "||AOrig||_1 = ||AOrig||_oo     = "
                 << infNormOfAOrig << "\n"
                 << "||AOrig||_F                    = "
                 << frobNormOfAOrig << "\n"
                 << "||A||_1 = ||A||_oo             = "
                 << infNormOfA << "\n"
                 << "||A||_F                        = "
                 << frobNormOfA << "\n"
                 << "||A X - U AOrig U^H X||_1  = "
                 << oneNormOfError << "\n"
                 << "||A X - U AOrig U^H X||_oo = " 
                 << infNormOfError << "\n"
                 << "||A X - U AOrig U^H X||_F  = "
                 << frobNormOfError << endl;
        }
    }
}

template<typename F> 
void TestTwoSidedTrmm
( bool testCorrectness, bool print, UpperOrLower uplo, UnitOrNonUnit diag, 
  int m, const Grid& g )
{
    DistMatrix<F> A(g), B(g), AOrig(g);

    Zeros( A, m, m );
    Zeros( B, m, m );
    MakeHermitianUniformSpectrum( A, 1, 10 );
    MakeHermitianUniformSpectrum( B, 1, 10 );
    MakeTriangular( uplo, B );
    if( testCorrectness )
    {
        if( g.Rank() == 0 )
        {
            cout << "  Making copy of original matrix...";
            cout.flush();
        }
        AOrig = A;
        if( g.Rank() == 0 )
            cout << "DONE" << endl;
    }
    if( print )
    {
        A.Print("A");
        B.Print("B");
    }

    if( g.Rank() == 0 )
    {
        cout << "  Starting reduction to Hermitian standard EVP...";
        cout.flush();
    }
    mpi::Barrier( g.Comm() );
    const double startTime = mpi::Time();
    TwoSidedTrmm( uplo, diag, A, B );
    mpi::Barrier( g.Comm() );
    const double runTime = mpi::Time() - startTime;
    double gFlops = Pow(double(m),3.)/(runTime*1.e9);
    if( IsComplex<F>::val )
        gFlops *= 4.;
    if( g.Rank() == 0 )
    {
        cout << "DONE. " << endl
             << "  Time = " << runTime << " seconds. GFlops = "
             << gFlops << endl;
    }
    if( print )
        A.Print("A after reduction");
    if( testCorrectness )
        TestCorrectness( print, uplo, diag, A, B, AOrig );
}

int 
main( int argc, char* argv[] )
{
    Initialize( argc, argv );
    mpi::Comm comm = mpi::COMM_WORLD;
    const int commRank = mpi::CommRank( comm );
    const int commSize = mpi::CommSize( comm );

    try
    {
        int r = Input("--r","height of process grid",0);
        const char uploChar = Input
            ("--uplo","lower or upper triangular storage: L/U",'L');
        const char diagChar = Input("--unit","(non-)unit diagonal: N/U",'N');
        const int m = Input("--m","height of matrix",100);
        const int nb = Input("--nb","algorithmic blocksize",96);
        const bool testCorrectness = Input
            ("--correctness","test correctness?",true);
        const bool print = Input("--print","print matrices?",false);
        ProcessInput();
        PrintInputReport();

        if( r == 0 )
            r = Grid::FindFactor( commSize );
        const int c = commSize / r;
        const Grid g( comm, r, c );
        const UpperOrLower uplo = CharToUpperOrLower( uploChar );
        const UnitOrNonUnit diag = CharToUnitOrNonUnit( diagChar );
        SetBlocksize( nb );

#ifndef RELEASE
        if( commRank == 0 )
        {
            cout << "==========================================\n"
                 << " In debug mode! Performance will be poor! \n"
                 << "==========================================" << endl;
        }
#endif
        if( commRank == 0 )
            cout << "Will test TwoSidedTrmm" << uploChar << diagChar << endl;

        if( commRank == 0 )
        {
            cout << "---------------------\n"
                 << "Testing with doubles:\n"
                 << "---------------------" << endl;
        }
        TestTwoSidedTrmm<double>( testCorrectness, print, uplo, diag, m, g );

        if( commRank == 0 )
        {
            cout << "--------------------------------------\n"
                 << "Testing with double-precision complex:\n"
                 << "--------------------------------------" << endl;
        }
        TestTwoSidedTrmm<Complex<double> >
        ( testCorrectness, print, uplo, diag, m, g );
    }
    catch( ArgException& e ) { }
    catch( exception& e )
    {
        ostringstream os;
        os << "Process " << commRank << " caught error message:\n" << e.what()
           << endl; 
        cerr << os.str();
#ifndef RELEASE
        DumpCallStack();
#endif
    }
    Finalize();
    return 0;
}
