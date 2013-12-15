/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef ELEM_IO_WRITE_HPP
#define ELEM_IO_WRITE_HPP

#include "elemental/io/Print.hpp"

#ifdef HAVE_QT5
# include <QFile>
# include <QImage>
# include <QPainter>
# include <QStylePainter>
#endif

namespace elem {
namespace write {

template<typename T>
inline void
Ascii
( const Matrix<T>& A, std::string filename="matrix.txt", std::string title="" )
{
    DEBUG_ONLY(CallStackEntry cse("write::Ascii"))
    std::ofstream file( filename.c_str() );
    file.setf( std::ios::scientific );
    Print( A, title, file );
}

template<typename T>
inline void
MatlabAscii
( const Matrix<T>& A, std::string filename="matrix.m", 
  std::string title="matrix" )
{
    DEBUG_ONLY(CallStackEntry cse("write::MatlabAscii"))
    // Empty titles are not legal
    if( title == "" )
        title = "matrix";

    std::ofstream file( filename.c_str() );
    file.setf( std::ios::scientific );
    file << title << " = [\n";
    Print( A, "", file );
    file << "];\n";
}

#ifdef HAVE_QT5
inline void
SaveQImage
( const QImage& image, FileFormat format=PNG, std::string basename="matrix" )
{
    DEBUG_ONLY(CallStackEntry cse("write::Image"))
    std::string filename;
    switch( format )
    {
    case BMP:  
    {
        filename = basename + ".bmp";
        QFile file( filename.c_str() );
        file.open( QIODevice::WriteOnly );
        image.save( &file, "BMP" ); 
        break;
    }
    case JPG:  
    {
        filename = basename + ".jpg";
        QFile file( filename.c_str() );
        file.open( QIODevice::WriteOnly );
        image.save( &file, "JPG" ); 
        break;
    }
    case JPEG: 
    {
        filename = basename + ".jpeg";
        QFile file( filename.c_str() );
        file.open( QIODevice::WriteOnly );
        image.save( &file, "JPEG" ); 
        break;
    }
    case PNG:  
    {
        filename = basename + ".png";
        QFile file( filename.c_str() );
        file.open( QIODevice::WriteOnly );
        image.save( &file, "PNG" ); 
        break;
    }
    case PPM:  
    {
        filename = basename + ".ppm";
        QFile file( filename.c_str() );
        file.open( QIODevice::WriteOnly );
        image.save( &file, "PPM" ); 
        break;
    }
    case XBM:  
    {
        filename = basename + ".xbm";
        QFile file( filename.c_str() );
        file.open( QIODevice::WriteOnly );
        image.save( &file, "XBM" ); 
        break;
    }
    case XPM:  
    {
        filename = basename + ".xpm";
        QFile file( filename.c_str() );
        file.open( QIODevice::WriteOnly );
        image.save( &file, "XPM" ); 
        break;
    }
    default: LogicError("Invalid image type");
    }
}
#endif // ifdef HAVE_QT5

template<typename T>
inline void
RealPartImage
( const Matrix<T>& A, FileFormat format=PNG, std::string basename="matrix" )
{
    DEBUG_ONLY(CallStackEntry cse("write::RealPartImage"))
#ifdef HAVE_QT5
    typedef Base<T> Real;
    const Int m = A.Height();
    const Int n = A.Width();

    // Compute the maximum and minimum values
    Real minVal=0, maxVal=0; 
    if( m != 0 && n != 0 )
    {
        minVal = maxVal = A.GetRealPart( 0, 0 );
        for( Int j=0; j<n; ++j )
        {
            for( Int i=0; i<m; ++i )
            {
                minVal = std::min( minVal, A.GetRealPart(i,j) );
                maxVal = std::max( maxVal, A.GetRealPart(i,j) );
            }
        }
    }

    // TODO: Parameterize these instead
    const Int mPix = Max( 500, 2*m );
    const Int nPix = Max( 500, 2*n );
    const double mRatio = double(m) / double(mPix);
    const double nRatio = double(n) / double(nPix);
    QImage image( nPix, mPix, QImage::Format_RGB32 );
    for( Int jPix=0; jPix<nPix; ++jPix )
    {
        const Int j = nRatio*jPix;
        for( Int iPix=0; iPix<mPix; ++iPix )
        {
            const Int i = mRatio*iPix;
            QRgb color = SampleColorMap( A.GetRealPart(i,j), minVal, maxVal );
            image.setPixel( jPix, iPix, color );
        }
    }

    SaveQImage( image, format, basename );
#else
    LogicError("Qt5 not available");
#endif // ifdef HAVE_QT5
}

template<typename T>
inline void
ImagPartImage
( const Matrix<T>& A, FileFormat format=PNG, std::string basename="matrix" )
{
    DEBUG_ONLY(CallStackEntry cse("write::ImagPartImage"))
#ifdef HAVE_QT5
    typedef Base<T> Real;
    const Int m = A.Height();
    const Int n = A.Width();

    // Compute the maximum and minimum values
    Real minVal=0, maxVal=0; 
    if( m != 0 && n != 0 )
    {
        minVal = maxVal = A.GetImagPart( 0, 0 );
        for( Int j=0; j<n; ++j )
        {
            for( Int i=0; i<m; ++i )
            {
                minVal = std::min( minVal, A.GetImagPart(i,j) );
                maxVal = std::max( maxVal, A.GetImagPart(i,j) );
            }
        }
    }

    // TODO: Parameterize these instead
    const Int mPix = Max( 500, 2*m );
    const Int nPix = Max( 500, 2*n );
    const double mRatio = double(m) / double(mPix);
    const double nRatio = double(n) / double(nPix);
    QImage image( nPix, mPix, QImage::Format_RGB32 );
    for( Int jPix=0; jPix<nPix; ++jPix )
    {
        const Int j = nRatio*jPix;
        for( Int iPix=0; iPix<mPix; ++iPix )
        {
            const Int i = mRatio*iPix;
            QRgb color = SampleColorMap( A.GetImagPart(i,j), minVal, maxVal );
            image.setPixel( jPix, iPix, color );
        }
    }

    SaveQImage( image, format, basename );
#else
    LogicError("Qt5 not available");
#endif // ifdef HAVE_QT5
}

template<typename Real>
inline void
Image
( const Matrix<Real>& A, FileFormat format=PNG, std::string basename="matrix" )
{
    DEBUG_ONLY(CallStackEntry cse("write::Image"))
    RealPartImage( A, format, basename );
}

template<typename Real>
inline void
Image
( const Matrix<Complex<Real>>& A, FileFormat format=PNG, 
  std::string basename="matrix" )
{
    DEBUG_ONLY(CallStackEntry cse("write::Image"))
    std::string realBasename = basename + "_real";
    std::string imagBasename = basename + "_imag";
    RealPartImage( A, format, realBasename );
    ImagPartImage( A, format, imagBasename );
}

} // namespace write

template<typename T>
inline void
Write
( const Matrix<T>& A, FileFormat format=ASCII, 
  std::string basename="matrix", std::string title="" )
{
    DEBUG_ONLY(CallStackEntry cse("Write"))
    std::string filename;
    switch( format )
    {
    case ASCII:
        filename = basename + ".txt";
        write::Ascii( A, filename, title );
        break;
    case MATLAB_ASCII:
        filename = basename + ".m";
        write::MatlabAscii( A, filename, title );
        break;
    case BMP:
    case JPG:
    case JPEG:
    case PNG:
    case PPM:
    case XBM:
    case XPM:
        write::Image( A, format, basename );
        break;
    }
}

template<typename T,Distribution U,Distribution V>
inline void
Write
( const DistMatrix<T,U,V>& A, FileFormat format=ASCII, 
  std::string basename="matrix", std::string title="" )
{
    DEBUG_ONLY(CallStackEntry cse("Write"))
    DistMatrix<T,CIRC,CIRC> A_CIRC_CIRC( A );
    if( A_CIRC_CIRC.CrossRank() == A_CIRC_CIRC.Root() )
        Write( A_CIRC_CIRC.LockedMatrix(), format, basename, title );
}

// If already in [* ,* ] or [o ,o ] distributions, no copy is needed

template<typename T>
inline void
Write
( const DistMatrix<T,STAR,STAR>& A, FileFormat format=ASCII,
  std::string basename="matrix", std::string title="" )
{
    DEBUG_ONLY(CallStackEntry cse("Write"))
    if( A.Grid().VCRank() == 0 )
        Write( A.LockedMatrix(), format, basename, title );
}
template<typename T>
inline void
Write
( const DistMatrix<T,CIRC,CIRC>& A, FileFormat format=ASCII,
  std::string basename="matrix", std::string title="" )
{
    DEBUG_ONLY(CallStackEntry cse("Write"))
    if( A.CrossRank() == A.Root() )
        Write( A.LockedMatrix(), format, basename, title );
}

} // namespace elem

#endif // ifndef ELEM_IO_WRITE_HPP
