/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "elemental-lite.hpp"
#include "elemental/blas-like/level1/Axpy.hpp"
#include "elemental/blas-like/level1/Transpose.hpp"

namespace elem {

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::DistMatrix( const elem::Grid& grid )
: AbstractDistMatrix<T,Int>
  (0,0,false,false,0,0,
   (grid.InGrid() ? grid.Row() : 0),
   (grid.InGrid() ? grid.Col() : 0),
    0,0,grid)
{ }

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::DistMatrix
( Int height, Int width, const elem::Grid& grid )
: AbstractDistMatrix<T,Int>
  (height,width,false,false,0,0,
   (grid.InGrid() ? grid.Row() : 0),
   (grid.InGrid() ? grid.Col() : 0),
   (grid.InGrid() ? Length(height,grid.Row(),0,grid.Height()) : 0),
   (grid.InGrid() ? Length(width,grid.Col(),0,grid.Width()) : 0),
    grid)
{ }

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::DistMatrix
( Int height, Int width, 
  Int colAlignment, Int rowAlignment, const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,true,true,colAlignment,rowAlignment,
   (g.InGrid() ? Shift(g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Shift(g.Col(),rowAlignment,g.Width()) : 0),
   (g.InGrid() ? Length(height,g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Length(width,g.Col(),rowAlignment,g.Width()) : 0),
   g)
{ }

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::DistMatrix
( Int height, Int width,
  Int colAlignment, Int rowAlignment, Int ldim, const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,true,true,colAlignment,rowAlignment,
   (g.InGrid() ? Shift(g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Shift(g.Col(),rowAlignment,g.Width()) : 0),
   (g.InGrid() ? Length(height,g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Length(width,g.Col(),rowAlignment,g.Width()) : 0),
   ldim,g)
{ }

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::DistMatrix
( Int height, Int width, Int colAlignment, Int rowAlignment,
  const T* buffer, Int ldim, const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,
   colAlignment,rowAlignment,
   (g.InGrid() ? Shift(g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Shift(g.Col(),rowAlignment,g.Width()) : 0),
   (g.InGrid() ? Length(height,g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Length(width,g.Col(),rowAlignment,g.Width()) : 0),
   buffer,ldim,g)
{ }

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::DistMatrix
( Int height, Int width, Int colAlignment, Int rowAlignment,
  T* buffer, Int ldim, const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,
   colAlignment,rowAlignment,
   (g.InGrid() ? Shift(g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Shift(g.Col(),rowAlignment,g.Width()) : 0),
   (g.InGrid() ? Length(height,g.Row(),colAlignment,g.Height()) : 0),
   (g.InGrid() ? Length(width,g.Col(),rowAlignment,g.Width()) : 0),
   buffer,ldim,g)
{ }

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::DistMatrix( const DistMatrix<T,MC,MR,Int>& A )
: AbstractDistMatrix<T,Int>
  (0,0,false,false,0,0,
   (A.Participating() ? A.ColRank() : 0),
   (A.Participating() ? A.RowRank() : 0),
   0,0,A.Grid())
{
#ifndef RELEASE
    CallStackEntry entry("DistMatrix[MC,MR]::DistMatrix");
#endif
    if( &A != this )
        *this = A;
    else
        throw std::logic_error("Tried to construct [MC,MR] with itself");
}

template<typename T,typename Int>
template<Distribution U,Distribution V>
DistMatrix<T,MC,MR,Int>::DistMatrix( const DistMatrix<T,U,V,Int>& A )
: AbstractDistMatrix<T,Int>
  (0,0,false,false,0,0,
   (A.Participating() ? A.ColRank() : 0),
   (A.Participating() ? A.RowRank() : 0),
   0,0,A.Grid())
{
#ifndef RELEASE
    CallStackEntry entry("DistMatrix[MC,MR]::DistMatrix");
#endif
    if( MC != U || MR != V ||
        reinterpret_cast<const DistMatrix<T,MC,MR,Int>*>(&A) != this )
        *this = A;
    else
        throw std::logic_error("Tried to construct [MC,MR] with itself");
}

template<typename T,typename Int>
DistMatrix<T,MC,MR,Int>::~DistMatrix()
{ }

template<typename T,typename Int>
elem::DistData<Int>
DistMatrix<T,MC,MR,Int>::DistData() const
{
    elem::DistData<Int> data;
    data.colDist = MC;
    data.rowDist = MR;
    data.colAlignment = this->colAlignment_;
    data.rowAlignment = this->rowAlignment_;
    data.diagPath = 0;
    data.grid = this->grid_;
    return data;
}

template<typename T,typename Int>
Int
DistMatrix<T,MC,MR,Int>::ColStride() const
{ return this->grid_->Height(); }

template<typename T,typename Int>
Int
DistMatrix<T,MC,MR,Int>::RowStride() const
{ return this->grid_->Width(); }

template<typename T,typename Int>
Int
DistMatrix<T,MC,MR,Int>::ColRank() const
{ return this->grid_->Row(); }

template<typename T,typename Int>
Int
DistMatrix<T,MC,MR,Int>::RowRank() const
{ return this->grid_->Col(); }

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AlignWith( const elem::DistData<Int>& data )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::AlignWith");
    this->AssertFreeColAlignment();
    this->AssertFreeRowAlignment();
#endif
    const Grid& grid = *data.grid;
    this->SetGrid( grid );

    if( data.colDist == MC && data.rowDist == MR )
    {
        this->colAlignment_ = data.colAlignment;
        this->rowAlignment_ = data.rowAlignment;
        this->constrainedColAlignment_ = true;
        this->constrainedRowAlignment_ = true;
    }
    else if( data.colDist == MC && data.rowDist == STAR )
    {
        this->colAlignment_ = data.colAlignment;
        this->constrainedColAlignment_ = true; 
    }
    else if( data.colDist == MR && data.rowDist == MC )
    {
        this->colAlignment_ = data.rowAlignment;
        this->rowAlignment_ = data.colAlignment;
        this->constrainedColAlignment_ = true;
        this->constrainedRowAlignment_ = true;
    }
    else if( data.colDist == MR && data.rowDist == STAR )
    {
        this->rowAlignment_ = data.colAlignment;
        this->constrainedRowAlignment_ = true;
    }
    else if( data.colDist == STAR && data.rowDist == MC )
    {
        this->colAlignment_ = data.rowAlignment;
        this->constrainedColAlignment_ = true;
    }
    else if( data.colDist == STAR && data.rowDist == MR )
    {
        this->rowAlignment_ = data.rowAlignment;
        this->constrainedRowAlignment_ = true;
    }
    else if( data.colDist == STAR && data.rowDist == VC )
    {
        this->colAlignment_ = data.rowAlignment % this->ColStride();
        this->constrainedColAlignment_ = true;
    }
    else if( data.colDist == STAR && data.rowDist == VR )
    {
        this->rowAlignment_ = data.rowAlignment % this->RowStride();
        this->constrainedRowAlignment_ = true;
    }
    else if( data.colDist == VC && data.rowDist == STAR )
    {
        this->colAlignment_ = data.colAlignment % this->ColStride();
        this->constrainedColAlignment_ = true;
    }
    else if( data.colDist == VR && data.rowDist == STAR )
    {
        this->rowAlignment_ = data.colAlignment % this->RowStride();
        this->constrainedRowAlignment_ = true;
    }
#ifndef RELEASE
    else throw std::logic_error("Nonsensical alignment");
#endif
    this->SetShifts();
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AlignWith( const AbstractDistMatrix<T,Int>& A )
{ this->AlignWith( A.DistData() ); }

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AlignColsWith( const elem::DistData<Int>& data )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::AlignColsWith");
    this->AssertFreeColAlignment();
    if( *this->grid_ != *data.grid )
        throw std::logic_error("Grids do not match");
#endif
    if( data.colDist == MC )
        this->colAlignment_ = data.colAlignment;
    else if( data.rowDist == MC )
        this->colAlignment_ = data.rowAlignment;
    else if( data.colDist == VC )
        this->colAlignment_ = data.colAlignment % this->ColStride();
    else if( data.rowDist == VC )
        this->colAlignment_ = data.rowAlignment % this->ColStride();
#ifndef RELEASE
    else throw std::logic_error("Nonsensical alignment");
#endif
    this->constrainedColAlignment_ = true;
    this->SetShifts();
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AlignColsWith( const AbstractDistMatrix<T,Int>& A )
{ this->AlignColsWith( A.DistData() ); }

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AlignRowsWith( const elem::DistData<Int>& data )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::AlignRowsWith");
    this->AssertFreeRowAlignment();
    if( *this->grid_ != *data.grid )
        throw std::logic_error("Grids do not match");
#endif
    if( data.colDist == MR )
        this->rowAlignment_ = data.colAlignment;
    else if( data.rowDist == MR )
        this->rowAlignment_ = data.rowAlignment;
    else if( data.colDist == VR )
        this->rowAlignment_ = data.colAlignment % this->RowStride();
    else if( data.rowDist == VR )
        this->rowAlignment_ = data.rowAlignment % this->RowStride();
#ifndef RELEASE
    else throw std::logic_error("Nonsensical alignment");
#endif
    this->constrainedRowAlignment_ = true;
    this->SetShifts();
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AlignRowsWith( const AbstractDistMatrix<T,Int>& A )
{ this->AlignRowsWith( A.DistData() ); }

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::PrintBase
( std::ostream& os, const std::string msg ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::PrintBase");
#endif
    const elem::Grid& g = this->Grid();

    const Int r = g.Height();
    const Int c = g.Width();

    if( g.Rank() == 0 && msg != "" )
        os << msg << std::endl;

    const Int height = this->Height();
    const Int width  = this->Width();
    const Int localHeight = this->LocalHeight();
    const Int localWidth  = this->LocalWidth();
    const Int colShift = this->ColShift();
    const Int rowShift = this->RowShift();

    if( height == 0 || width == 0  || !g.InGrid() )
        return;

    // Fill the send buffer: zero it then place our entries into their 
    // appropriate locations
    std::vector<T> sendBuf(height*width,0);
    const T* thisBuffer = this->LockedBuffer();
    const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        T* destCol = &sendBuf[colShift+(rowShift+jLocal*c)*height];
        const T* sourceCol = &thisBuffer[jLocal*thisLDim];
        for( Int iLocal=0; iLocal<localHeight; ++iLocal )
            destCol[iLocal*r] = sourceCol[iLocal];
    }

    // If we are the root, allocate a receive buffer
    std::vector<T> recvBuf;
    if( g.Rank() == 0 )
        recvBuf.resize( height*width );

    // Sum the contributions and send to the root
    mpi::Reduce
    ( &sendBuf[0], &recvBuf[0], height*width, mpi::SUM, 0, g.VCComm() );

    if( g.Rank() == 0 )
    {
        // Print the data
        for( Int i=0; i<height; ++i )
        {
            for( Int j=0; j<width; ++j )
                os << recvBuf[i+j*height] << " ";
            os << "\n";
        }
        os << std::endl;
    }
    mpi::Barrier( g.VCComm() );
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::Attach
( Int height, Int width, Int colAlignment, Int rowAlignment, 
  T* buffer, Int ldim, const elem::Grid& g )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::Attach");
#endif
    this->Empty();

    this->grid_ = &g;
    this->height_ = height;
    this->width_ = width;
    this->colAlignment_ = colAlignment;
    this->rowAlignment_ = rowAlignment;
    this->viewing_ = true;
    this->SetShifts();
    if( g.InGrid() )
    {
        Int localHeight = Length(height,this->colShift_,this->ColStride());
        Int localWidth = Length(width,this->rowShift_,this->RowStride());
        this->matrix_.Attach( localHeight, localWidth, buffer, ldim );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::LockedAttach
( Int height, Int width, Int colAlignment, Int rowAlignment, 
  const T* buffer, Int ldim, const elem::Grid& g )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::LockedAttach");
#endif
    this->Empty();

    this->grid_ = &g;
    this->height_ = height;
    this->width_ = width;
    this->colAlignment_ = colAlignment;
    this->rowAlignment_ = rowAlignment;
    this->viewing_ = true;
    this->locked_ = true;
    this->SetShifts();
    if( g.InGrid() )
    {
        Int localHeight = Length(height,this->colShift_,this->ColStride());
        Int localWidth = Length(width,this->rowShift_,this->RowStride());
        this->matrix_.LockedAttach( localHeight, localWidth, buffer, ldim );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::ResizeTo( Int height, Int width )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::ResizeTo");
    this->AssertNotLocked();
#endif
    this->height_ = height;
    this->width_ = width;
    if( this->Participating() )
        this->matrix_.ResizeTo
        ( Length(height,this->ColShift(),this->ColStride()),
          Length(width, this->RowShift(),this->RowStride()) );
}

template<typename T,typename Int>
T
DistMatrix<T,MC,MR,Int>::Get( Int i, Int j ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::Get");
    this->AssertValidEntry( i, j );
#endif
    // We will determine the owner of the (i,j) entry and have him Broadcast
    // throughout the entire process grid
    const Int ownerRow = (i + this->ColAlignment()) % this->ColStride();
    const Int ownerCol = (j + this->RowAlignment()) % this->RowStride();
    const Int ownerRank = ownerRow + ownerCol*this->ColStride();

    T u;
    const elem::Grid& g = this->Grid();
    if( g.VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / this->ColStride();
        const Int jLocal = (j-this->RowShift()) / this->RowStride();
        u = this->GetLocal(iLocal,jLocal);
    }
    mpi::Broadcast( &u, 1, g.VCToViewingMap(ownerRank), g.ViewingComm() );
    return u;
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::Set( Int i, Int j, T u )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::Set");
    this->AssertValidEntry( i, j );
#endif
    const Int ownerRow = (i + this->ColAlignment()) % this->ColStride();
    const Int ownerCol = (j + this->RowAlignment()) % this->RowStride();
    const Int ownerRank = ownerRow + ownerCol*this->ColStride();
    if( this->Grid().VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / this->ColStride();
        const Int jLocal = (j-this->RowShift()) / this->RowStride();
        this->SetLocal(iLocal,jLocal,u);
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::Update( Int i, Int j, T u )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::Update");
    this->AssertValidEntry( i, j );
#endif
    const Int ownerRow = (i + this->ColAlignment()) % this->ColStride();
    const Int ownerCol = (j + this->RowAlignment()) % this->RowStride();
    const Int ownerRank = ownerRow + ownerCol*this->ColStride();
    if( this->Grid().VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / this->ColStride();
        const Int jLocal = (j-this->RowShift()) / this->RowStride();
        this->UpdateLocal(iLocal,jLocal,u);
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::GetDiagonal
( DistMatrix<T,MD,STAR,Int>& d, Int offset ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetDiagonal");
    if( d.Viewing() )
        this->AssertSameGrid( d.Grid() );
#endif
    const Int diagLength = this->DiagonalLength(offset);
#ifndef RELEASE
    if( d.Viewing() && diagLength != d.Height() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << diagLength << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( ( d.Viewing() || d.ConstrainedColAlignment() ) &&
        !d.AlignedWithDiagonal( *this, offset ) )
    {
        std::ostringstream os;
        os << "offset:         " << offset << "\n"
           << "colAlignment:   " << this->colAlignment_ << "\n"
           << "rowAlignment:   " << this->rowAlignment_ << "\n"
           << "d.diagPath:     " << d.diagPath_ << "\n"
           << "d.colAlignment: " << d.colAlignment_ << std::endl;
        std::cerr << os.str();
        throw std::logic_error("d must be aligned with the 'offset' diagonal");
    }
#endif
    const elem::Grid& g = this->Grid();
    if( !d.Viewing() )
    {
        d.SetGrid( g );
        if( !d.ConstrainedColAlignment() )
            d.AlignWithDiagonal( *this, offset );
        d.ResizeTo( diagLength, 1 );
    }
    if( !d.Participating() )
        return;

    Int iStart, jStart;
    const Int diagShift = d.ColShift();
    if( offset >= 0 )
    {
        iStart = diagShift;
        jStart = diagShift+offset;
    }
    else
    {
        iStart = diagShift-offset;
        jStart = diagShift;
    }

    const Int colStride = this->ColStride();
    const Int rowStride = this->RowStride();
    const Int colShift = this->ColShift();
    const Int rowShift = this->RowShift();
    const Int iLocalStart = (iStart-colShift) / colStride;
    const Int jLocalStart = (jStart-rowShift) / rowStride;

    const Int lcm = g.LCM();
    const Int localDiagLength = d.LocalHeight();
    T* dBuffer = d.Buffer();
    const T* buffer = this->LockedBuffer();
    const Int ldim = this->LDim();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int k=0; k<localDiagLength; ++k )
    {
        const Int iLocal = iLocalStart + k*(lcm/colStride);
        const Int jLocal = jLocalStart + k*(lcm/rowStride);
        dBuffer[k] = buffer[iLocal+jLocal*ldim];
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::GetDiagonal
( DistMatrix<T,STAR,MD,Int>& d, Int offset ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetDiagonal");
    if( d.Viewing() )
        this->AssertSameGrid( d.Grid() );
#endif
    const Int diagLength = this->DiagonalLength(offset);
#ifndef RELEASE
    if( d.Viewing() && diagLength != d.Width() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << diagLength << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( ( d.Viewing() || d.ConstrainedRowAlignment() ) &&
        !d.AlignedWithDiagonal( *this, offset ) )
        throw std::logic_error("d must be aligned with the 'offset' diagonal");
#endif
    const elem::Grid& g = this->Grid();
    if( !d.Viewing() )
    {
        d.SetGrid( g );
        if( !d.ConstrainedRowAlignment() )
            d.AlignWithDiagonal( *this, offset );
        d.ResizeTo( 1, diagLength );
    }
    if( !d.Participating() )
        return;

    Int iStart, jStart;
    const Int diagShift = d.RowShift();
    if( offset >= 0 )
    {
        iStart = diagShift;
        jStart = diagShift+offset;
    }
    else
    {
        iStart = diagShift-offset;
        jStart = diagShift;
    }

    const Int colStride = this->ColStride();
    const Int rowStride = this->RowStride();
    const Int colShift = this->ColShift();
    const Int rowShift = this->RowShift();
    const Int iLocalStart = (iStart-colShift) / colStride;
    const Int jLocalStart = (jStart-rowShift) / rowStride;

    const Int localDiagLength = d.LocalWidth();
    T* dBuffer = d.Buffer();
    const Int dLDim = d.LDim();
    const T* buffer = this->LockedBuffer();
    const Int ldim = this->LDim();
    const Int lcm = g.LCM();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int k=0; k<localDiagLength; ++k )
    {
        const Int iLocal = iLocalStart + k*(lcm/colStride);
        const Int jLocal = jLocalStart + k*(lcm/rowStride);
        dBuffer[k*dLDim] = buffer[iLocal+jLocal*ldim];
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetDiagonal
( const DistMatrix<T,MD,STAR,Int>& d, Int offset )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetDiagonal");
    this->AssertSameGrid( d.Grid() );
    if( d.Width() != 1 )
        throw std::logic_error("d must be a column vector");
    const Int diagLength = this->DiagonalLength(offset);
    if( diagLength != d.Height() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << diagLength << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( !d.AlignedWithDiagonal( *this, offset ) )
        throw std::logic_error("d must be aligned with the 'offset' diagonal");
#endif
    if( !d.Participating() )
        return;

    Int iStart,jStart;
    const Int diagShift = d.ColShift();
    if( offset >= 0 )
    {
        iStart = diagShift;
        jStart = diagShift+offset;
    }
    else
    {
        iStart = diagShift-offset;
        jStart = diagShift;
    }

    const Int colStride = this->ColStride();
    const Int rowStride = this->RowStride();
    const Int colShift = this->ColShift();
    const Int rowShift = this->RowShift();
    const Int iLocalStart = (iStart-colShift) / colStride;
    const Int jLocalStart = (jStart-rowShift) / rowStride;

    const Int localDiagLength = d.LocalHeight();
    const T* dBuffer = d.LockedBuffer();
    T* buffer = this->Buffer();
    const Int ldim = this->LDim();
    const Int lcm = this->Grid().LCM();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int k=0; k<localDiagLength; ++k )
    {
        const Int iLocal = iLocalStart + k*(lcm/colStride);
        const Int jLocal = jLocalStart + k*(lcm/rowStride);
        buffer[iLocal+jLocal*ldim] = dBuffer[k];
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetDiagonal
( const DistMatrix<T,STAR,MD,Int>& d, Int offset )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetDiagonal");
    this->AssertSameGrid( d.Grid() );
    if( d.Height() != 1 )
        throw std::logic_error("d must be a row vector");
    const Int diagLength = this->DiagonalLength(offset);
    if( diagLength != d.Width() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << diagLength << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( !d.AlignedWithDiagonal( *this, offset ) )
        throw std::logic_error("d must be aligned with the 'offset' diagonal");
#endif
    if( !d.Participating() )
        return;

    Int iStart,jStart;
    const Int diagShift = d.RowShift();
    if( offset >= 0 )
    {
        iStart = diagShift;
        jStart = diagShift+offset;
    }
    else
    {
        iStart = diagShift-offset;
        jStart = diagShift;
    }

    const Int colStride = this->ColStride();
    const Int rowStride = this->RowStride();
    const Int colShift = this->ColShift();
    const Int rowShift = this->RowShift();
    const Int iLocalStart = (iStart-colShift) / colStride;
    const Int jLocalStart = (jStart-rowShift) / rowStride;

    const Int localDiagLength = d.LocalWidth();
    const T* dBuffer = d.LockedBuffer();
    T* buffer = this->Buffer();
    const Int dLDim = d.LDim();
    const Int ldim = this->LDim();
    const Int lcm = this->Grid().LCM();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int k=0; k<localDiagLength; ++k )
    {
        const Int iLocal = iLocalStart + k*(lcm/colStride);
        const Int jLocal = jLocalStart + k*(lcm/rowStride);
        buffer[iLocal+jLocal*ldim] = dBuffer[k*dLDim];
    }
}

//
// Utility functions, e.g., TransposeFrom
//

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AdjointFrom( const DistMatrix<T,STAR,MC,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::AdjointFrom");
#endif
    this->TransposeFrom( A, true );
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::TransposeFrom
( const DistMatrix<T,STAR,MC,Int>& A, bool conjugate )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::TransposeFrom");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Width(), A.Height() );
#endif
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->colAlignment_ = A.RowAlignment();
            this->SetColShift();
        }
        this->ResizeTo( A.Width(), A.Height() );
    }
    if( !this->Participating() )
        return;

    if( this->ColAlignment() == A.RowAlignment() )
    {
        const Int rowStride = this->RowStride();
        const Int rowShift = this->RowShift();
        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
        T* buffer = this->Buffer();
        const Int ldim = this->LDim();
        if( conjugate )
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &buffer[jLocal*ldim];
                const T* sourceCol = &ABuffer[rowShift+jLocal*rowStride];
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    destCol[iLocal] = Conj( sourceCol[iLocal*ALDim] );
            }
        }
        else
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &buffer[jLocal*ldim];
                const T* sourceCol = &ABuffer[rowShift+jLocal*rowStride];
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*ALDim];
            }
        }
    }
    else
    {
        const Grid& g = this->Grid();
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [MC,MR]::TransposeFrom." << std::endl;
#endif
        const Int colStride = this->ColStride();
        const Int rowStride = this->RowStride();
        const Int colRank = this->ColRank();
        const Int rowShift = this->RowShift();
        const Int colAlign = this->ColAlignment();
        const Int rowAlignA = A.RowAlignment();
        const Int sendRank = (colRank+colStride+colAlign-rowAlignA) % colStride;
        const Int recvRank = (colRank+colStride+rowAlignA-colAlign) % colStride;

        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const Int localWidthA = A.LocalWidth();
        const Int sendSize = localWidthA*localWidth;
        const Int recvSize = localHeight*localWidth;
        this->auxMemory_.Require( sendSize + recvSize );
        T* auxBuffer = this->auxMemory_.Buffer();
        T* sendBuffer = &auxBuffer[0];
        T* recvBuffer = &auxBuffer[sendSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
        if( conjugate )
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &sendBuffer[jLocal*localWidth];
                const T* sourceCol = &ABuffer[rowShift+jLocal*rowStride];
                for( Int iLocal=0; iLocal<localWidthA; ++iLocal )
                    destCol[iLocal] = Conj( sourceCol[iLocal*ALDim] );
            }
        }
        else
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &sendBuffer[jLocal*localWidth];
                const T* sourceCol = &ABuffer[rowShift+jLocal*rowStride];
                for( Int iLocal=0; iLocal<localWidthA; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*ALDim];
            }
        }

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendRank, 0,
          recvBuffer, recvSize, recvRank, mpi::ANY_TAG, g.ColComm() );

        // Unpack
        T* buffer = this->Buffer();
        const Int ldim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*localHeight];
            T* col = &buffer[jLocal*ldim];
            MemCopy( col, recvBufferCol, localHeight );
        }
        this->auxMemory_.Release();
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AdjointFrom
( const DistMatrix<T,MR,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::AdjointFrom");
#endif
    this->TransposeFrom( A, true );
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::TransposeFrom
( const DistMatrix<T,MR,STAR,Int>& A, bool conjugate )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::TransposeFrom");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Width(), A.Height() );
#endif
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->rowAlignment_ = A.ColAlignment();
            this->SetRowShift();
        }
        this->ResizeTo( A.Width(), A.Height() );
    }
    if( this->rowAlignment_ != A.ColAlignment() )
        throw std::logic_error("Unaligned TransposeFrom");

    if( this->Participating() ) 
    { 
        const Int colStride = this->ColStride();
        const Int colShift = this->ColShift();
        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
        T* buffer = this->Buffer();
        const Int ldim = this->LDim();
        if( conjugate )
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &buffer[jLocal*ldim];
                const T* sourceCol = &ABuffer[jLocal+colShift*ALDim];
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    destCol[iLocal] = Conj( sourceCol[iLocal*colStride*ALDim] );
            }
        }
        else
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &buffer[jLocal*ldim];
                const T* sourceCol = &ABuffer[jLocal+colShift*ALDim];
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*colStride*ALDim];
            }
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AdjointSumScatterFrom
( const DistMatrix<T,MR,STAR,Int>& AAdj_MR_STAR )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::AdjointSumScatterFrom");
#endif
    this->TransposeSumScatterFrom( AAdj_MR_STAR, true );
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::TransposeSumScatterFrom
( const DistMatrix<T,MR,STAR,Int>& ATrans_MR_STAR, bool conjugate )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::TransposeSumScatterFrom");
    this->AssertNotLocked();
    this->AssertSameGrid( ATrans_MR_STAR.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( ATrans_MR_STAR.Width(), ATrans_MR_STAR.Height() );
#endif
    const Grid& g = ATrans_MR_STAR.Grid();
    DistMatrix<T,MR,MC,Int> ATrans( g );
    if( this->Viewing() )
        ATrans.AlignWith( *this );
    ATrans.SumScatterFrom( ATrans_MR_STAR );
    Transpose( ATrans, *this, conjugate );
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::AdjointSumScatterUpdate
( T alpha, const DistMatrix<T,MR,STAR,Int>& AAdj_MR_STAR )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::AdjointSumScatterUpdate");
#endif
    this->TransposeSumScatterUpdate( alpha, AAdj_MR_STAR, true );
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::TransposeSumScatterUpdate
( T alpha, const DistMatrix<T,MR,STAR,Int>& ATrans_MR_STAR, bool conjugate )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::TransposeSumScatterUpdate");
    this->AssertNotLocked();
    this->AssertSameGrid( ATrans_MR_STAR.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( ATrans_MR_STAR.Width(), ATrans_MR_STAR.Height() );
#endif
    const Grid& g = ATrans_MR_STAR.Grid();
    DistMatrix<T,MR,MC,Int> ATrans( g );
    ATrans.SumScatterFrom( ATrans_MR_STAR );
    DistMatrix<T,MC,MR,Int> A( g );
    if( this->Viewing() )
        A.AlignWith( *this );
    Transpose( ATrans, A, conjugate );
    Axpy( alpha, A, *this );
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,MC,MR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [MC,MR]");
    this->AssertNotLocked();
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    if( !this->Viewing() )
    {
        if( this->Grid() == A.Grid() )
        {
            if( !this->ConstrainedColAlignment() )
            {
                this->colAlignment_ = A.ColAlignment();
                if( this->Participating() )
                    this->colShift_ = A.ColShift();
            }
            if( !this->ConstrainedRowAlignment() )
            {
                this->rowAlignment_ = A.RowAlignment();
                if( this->Participating() )
                    this->rowShift_ = A.RowShift();
            }
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !this->Participating() && !A.Participating() )
        return *this;

    if( this->Grid() == A.Grid() )
    {
        if( this->ColAlignment() == A.ColAlignment() &&
            this->RowAlignment() == A.RowAlignment() )
        {
            this->matrix_ = A.LockedMatrix();
        }
        else
        {
            const elem::Grid& g = this->Grid();
#ifdef UNALIGNED_WARNINGS
            if( g.Rank() == 0 )
                std::cerr << "Unaligned [MC,MR] <- [MC,MR]." << std::endl;
#endif
            const Int colRank = this->ColRank();
            const Int rowRank = this->RowRank();
            const Int colStride = this->ColStride();
            const Int rowStride = this->RowStride();
            const Int colAlignment = this->ColAlignment();
            const Int rowAlignment = this->RowAlignment();
            const Int colAlignmentA = A.ColAlignment();
            const Int rowAlignmentA = A.RowAlignment();
            const Int colDiff = colAlignment - colAlignmentA;
            const Int rowDiff = rowAlignment - rowAlignmentA;
            const Int sendRow = (colRank + colStride + colDiff) % colStride;
            const Int recvRow = (colRank + colStride - colDiff) % colStride;
            const Int sendCol = (rowRank + rowStride + rowDiff) % rowStride;
            const Int recvCol = (rowRank + rowStride - rowDiff) % rowStride;
            const Int sendRank = sendRow + sendCol*colStride;
            const Int recvRank = recvRow + recvCol*colStride;

            const Int localHeight = this->LocalHeight();
            const Int localWidth = this->LocalWidth();
            const Int localHeightA = A.LocalHeight();
            const Int localWidthA = A.LocalWidth();
            const Int sendSize = localHeightA*localWidthA;
            const Int recvSize = localHeight*localWidth;
            this->auxMemory_.Require( sendSize + recvSize );
            T* auxBuffer = this->auxMemory_.Buffer();
            T* sendBuffer = &auxBuffer[0];
            T* recvBuffer = &auxBuffer[sendSize];

            // Pack
            const T* ABuffer = A.LockedBuffer();
            const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidthA; ++jLocal )
            {
                const T* ACol = &ABuffer[jLocal*ALDim];
                T* sendBufferCol = &sendBuffer[jLocal*localHeightA];
                MemCopy( sendBufferCol, ACol, localHeightA );
            }

            // Communicate
            mpi::SendRecv
            ( sendBuffer, sendSize, sendRank, 0,
              recvBuffer, recvSize, recvRank, mpi::ANY_TAG, g.VCComm() );

            // Unpack
            T* buffer = this->Buffer();
            const Int ldim = this->LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* recvBufferCol = &recvBuffer[jLocal*localHeight];
                T* col = &buffer[jLocal*ldim];
                MemCopy( col, recvBufferCol, localHeight );
            }
            this->auxMemory_.Release();
        }
    }
    else // the grids don't match
    {
        if( !mpi::CongruentComms( A.Grid().ViewingComm(), 
                                  this->Grid().ViewingComm() ) )
            throw std::logic_error
            ("Redistributing between nonmatching grids currently requires"
             " the viewing communicators to match.");

        // Compute the number of process rows and columns that each process 
        // needs to send to.
        const Int colStride = this->ColStride();
        const Int rowStride = this->RowStride();
        const Int colRank = this->ColRank();
        const Int rowRank = this->RowRank();
        const Int colStrideA = A.ColStride();
        const Int rowStrideA = A.RowStride();
        const Int colRankA = A.ColRank();
        const Int rowRankA = A.RowRank();
        const Int colGCD = GCD( colStride, colStrideA );
        const Int rowGCD = GCD( rowStride, rowStrideA );
        const Int colLCM = colStride*colStrideA / colGCD;
        const Int rowLCM = rowStride*rowStrideA / rowGCD;
        const Int numColSends = colStride / colGCD;
        const Int numRowSends = rowStride / rowGCD;
        const Int localColStride = colLCM / colStride;
        const Int localRowStride = rowLCM / rowStride;
        const Int localColStrideA = numColSends;
        const Int localRowStrideA = numRowSends;

        const Int colAlign = this->ColAlignment();
        const Int rowAlign = this->RowAlignment();
        const Int colAlignA = A.ColAlignment();
        const Int rowAlignA = A.RowAlignment();

        const bool inThisGrid = this->Participating();
        const bool inAGrid = A.Participating();

        const Int maxSendSize = 
            (A.Height()/(colStrideA*localColStrideA)+1) * 
            (A.Width()/(rowStrideA*localRowStrideA)+1);

        // Have each member of A's grid individually send to all numRow x numCol
        // processes in order, while the members of this grid receive from all 
        // necessary processes at each step.
        Int requiredMemory = 0;
        if( inAGrid )
            requiredMemory += maxSendSize;
        if( inThisGrid )
            requiredMemory += maxSendSize;
        this->auxMemory_.Require( requiredMemory );
        T* auxBuffer = this->auxMemory_.Buffer();
        Int offset = 0;
        T* sendBuffer = &auxBuffer[offset];
        if( inAGrid )
            offset += maxSendSize;
        T* recvBuffer = &auxBuffer[offset];

        Int recvRow = 0; // avoid compiler warnings...
        if( inAGrid )
            recvRow = (((colRankA+colStrideA-colAlignA) % colStrideA) + colAlign) % colStride;
        for( Int colSend=0; colSend<numColSends; ++colSend )
        {
            Int recvCol = 0; // avoid compiler warnings...
            if( inAGrid )
                recvCol = (((rowRankA+rowStrideA-rowAlignA) % rowStrideA) + rowAlign) % rowStride;
            for( Int rowSend=0; rowSend<numRowSends; ++rowSend )
            {
                mpi::Request sendRequest;
                // Fire off this round of non-blocking sends
                if( inAGrid )
                {
                    // Pack the data
                    Int sendHeight = Length( A.LocalHeight(), colSend, numColSends );
                    Int sendWidth = Length( A.LocalWidth(), rowSend, numRowSends );
                    const T* ABuffer = A.LockedBuffer();
                    const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
                    #pragma omp parallel for
#endif
                    for( Int jLocal=0; jLocal<sendWidth; ++jLocal )
                    {
                        const Int j = rowSend+jLocal*localRowStrideA;
                        for( Int iLocal=0; iLocal<sendHeight; ++iLocal )
                        {
                            const Int i = colSend+iLocal*localColStrideA;
                            sendBuffer[iLocal+jLocal*sendHeight] = ABuffer[i+j*ALDim];
                        }
                    }
                    // Send data
                    const Int recvVCRank = recvRow + recvCol*colStride;
                    const Int recvViewingRank = this->Grid().VCToViewingMap( recvVCRank );
                    mpi::ISend
                    ( sendBuffer, sendHeight*sendWidth, recvViewingRank,
                      0, this->Grid().ViewingComm(), sendRequest );
                }
                // Perform this round of recv's
                if( inThisGrid )
                {
                    const Int sendColOffset = (colSend*colStrideA+colAlignA) % colStrideA;
                    const Int recvColOffset = (colSend*colStrideA+colAlign) % colStride;
                    const Int sendRowOffset = (rowSend*rowStrideA+rowAlignA) % rowStrideA;
                    const Int recvRowOffset = (rowSend*rowStrideA+rowAlign) % rowStride;

                    const Int firstSendRow = (((colRank+colStride-recvColOffset)%colStride)+sendColOffset)%colStrideA;
                    const Int firstSendCol = (((rowRank+rowStride-recvRowOffset)%rowStride)+sendRowOffset)%rowStrideA;

                    const Int colShift = (colRank+colStride-recvColOffset)%colStride;
                    const Int rowShift = (rowRank+rowStride-recvRowOffset)%rowStride;
                    const Int numColRecvs = Length( colStrideA, colShift, colStride ); 
                    const Int numRowRecvs = Length( rowStrideA, rowShift, rowStride );

                    // Recv data
                    // For now, simply receive sequentially. Until we switch to 
                    // nonblocking recv's, we won't be using much of the 
                    // recvBuffer
                    Int sendRow = firstSendRow;
                    for( Int colRecv=0; colRecv<numColRecvs; ++colRecv )
                    {
                        const Int sendColShift = Shift( sendRow, colAlignA, colStrideA ) + colSend*colStrideA;
                        const Int sendHeight = Length( A.Height(), sendColShift, colLCM );
                        const Int localColOffset = (sendColShift-this->ColShift()) / colStride;

                        Int sendCol = firstSendCol;
                        for( Int rowRecv=0; rowRecv<numRowRecvs; ++rowRecv )
                        {
                            const Int sendRowShift = Shift( sendCol, rowAlignA, rowStrideA ) + rowSend*rowStrideA;
                            const Int sendWidth = Length( A.Width(), sendRowShift, rowLCM );
                            const Int localRowOffset = (sendRowShift-this->RowShift()) / rowStride;

                            const Int sendVCRank = sendRow+sendCol*colStrideA;
                            const Int sendViewingRank = A.Grid().VCToViewingMap( sendVCRank );

                            mpi::Recv
                            ( recvBuffer, sendHeight*sendWidth, sendViewingRank, 0, this->Grid().ViewingComm() );
                            
                            // Unpack the data
                            T* buffer = this->Buffer();
                            const Int ldim = this->LDim();
#ifdef HAVE_OPENMP
                            #pragma omp parallel for
#endif
                            for( Int jLocal=0; jLocal<sendWidth; ++jLocal )
                            {
                                const Int j = localRowOffset+jLocal*localRowStride;
                                for( Int iLocal=0; iLocal<sendHeight; ++iLocal )
                                {
                                    const Int i = localColOffset+iLocal*localColStride;
                                    buffer[i+j*ldim] = recvBuffer[iLocal+jLocal*sendHeight];
                                }
                            }
                            // Set up the next send col
                            sendCol = (sendCol + rowStride) % rowStrideA;
                        }
                        // Set up the next send row
                        sendRow = (sendRow + colStride) % colStrideA;
                    }
                }
                // Ensure that this round of non-blocking sends completes
                if( inAGrid )
                {
                    mpi::Wait( sendRequest );
                    recvCol = (recvCol + rowStrideA) % rowStride;
                }
            }
            if( inAGrid )
                recvRow = (recvRow + colStrideA) % colStride;
        }
        this->auxMemory_.Release();
    }
    return *this;
}

// PAUSED PASS HERE

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,MC,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [MC,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->colAlignment_ = A.ColAlignment();
            this->SetColShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return *this;

    if( this->ColAlignment() == A.ColAlignment() )
    {
        const Int c = g.Width();
        const Int rowShift = this->RowShift();

        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();

        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* ACol = &ABuffer[(rowShift+jLocal*c)*ALDim];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            MemCopy( thisCol, ACol, localHeight );
        }
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [MC,MR] <- [MC,* ]." << std::endl;
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int rank = g.Row();
        const Int rowShift = this->RowShift();
        const Int colAlignment = this->ColAlignment();
        const Int colAlignmentA = A.ColAlignment();

        const Int sendRank = (rank+r+colAlignment-colAlignmentA) % r;
        const Int recvRank = (rank+r+colAlignmentA-colAlignment) % r;

        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const Int localHeightA = A.LocalHeight();

        const Int sendSize = localHeightA*localWidth;
        const Int recvSize = localHeight*localWidth;

        this->auxMemory_.Require( sendSize + recvSize );

        T* buffer = this->auxMemory_.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* ACol = &ABuffer[(rowShift+jLocal*c)*ALDim];
            T* sendBufferCol = &sendBuffer[jLocal*localHeightA];
            MemCopy( sendBufferCol, ACol, localHeightA );
        }

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendRank, 0,
          recvBuffer, recvSize, recvRank, mpi::ANY_TAG, g.ColComm() );

        // Unpack
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*localHeight];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            MemCopy( thisCol, recvBufferCol, localHeight );
        }
        this->auxMemory_.Release();
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,STAR,MR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [* ,MR]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->rowAlignment_ = A.RowAlignment();
            this->SetRowShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return *this;

    if( this->RowAlignment() == A.RowAlignment() )
    {
        const Int r = g.Height();
        const Int colShift = this->ColShift();

        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();

        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for 
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            T* destCol = &thisBuffer[jLocal*thisLDim];
            const T* sourceCol = &ABuffer[colShift+jLocal*ALDim];
            for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                destCol[iLocal] = sourceCol[iLocal*r];
        }
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [MC,MR] <- [* ,MR]." << std::endl;
#endif
        const Int r = g.Height(); 
        const Int c = g.Width();
        const Int col = g.Col();
        const Int colShift = this->ColShift();
        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentA = A.RowAlignment();

        const Int sendCol = (col+c+rowAlignment-rowAlignmentA) % c;
        const Int recvCol = (col+c+rowAlignmentA-rowAlignment) % c;

        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const Int localWidthA = A.LocalWidth();

        const Int sendSize = localHeight*localWidthA;
        const Int recvSize = localHeight*localWidth;

        this->auxMemory_.Require( sendSize + recvSize );

        T* buffer = this->auxMemory_.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidthA; ++jLocal )
        {
            T* destCol = &sendBuffer[jLocal*localHeight];
            const T* sourceCol = &ABuffer[colShift+jLocal*ALDim];
            for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                destCol[iLocal] = sourceCol[iLocal*r];
        }

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendCol, 0,
          recvBuffer, recvSize, recvCol, mpi::ANY_TAG, g.RowComm() );

        // Unpack
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for  
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*localHeight];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            MemCopy( thisCol, recvBufferCol, localHeight );
        }
        this->auxMemory_.Release();
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,MD,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [MD,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    throw std::logic_error("[MC,MR] = [MD,* ] not yet implemented");
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,STAR,MD,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [* ,MD]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    throw std::logic_error("[MC,MR] = [* ,MD] not yet implemented");
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,MR,MC,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [MR,MC]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );
    if( !g.InGrid() )
        return *this;

    if( A.Width() == 1 )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = g.Size();
        const Int myRow = g.Row();
        const Int myCol = g.Col();
        const Int rankCM = g.VCRank();
        const Int rankRM = g.VRRank();
        const Int ownerCol = this->RowAlignment();
        const Int ownerRow = A.RowAlignment();
        const Int colAlignment = this->ColAlignment();
        const Int colAlignmentA = A.ColAlignment();
        const Int colShift = this->ColShift();
        const Int colShiftA = A.ColShift();

        const Int height = A.Height();
        const Int maxLocalHeight = MaxLength(height,p);

        const Int portionSize = std::max(maxLocalHeight,mpi::MIN_COLL_MSG);

        const Int colShiftVC = Shift(rankCM,colAlignment,p);
        const Int colShiftVRA = Shift(rankRM,colAlignmentA,p);
        const Int sendRankCM = (rankCM+(p+colShiftVRA-colShiftVC)) % p;
        const Int recvRankRM = (rankRM+(p+colShiftVC-colShiftVRA)) % p;
        const Int recvRankCM = (recvRankRM/c)+r*(recvRankRM%c);

        this->auxMemory_.Require( (r+c)*portionSize );
        T* buffer = this->auxMemory_.Buffer();
        T* sendBuf = &buffer[0];
        T* recvBuf = &buffer[c*portionSize];

        if( myRow == ownerRow )
        {
            // Pack
            const T* ABuffer = A.LockedBuffer();
#ifdef HAVE_OPENMP
            #pragma omp parallel for  
#endif
            for( Int k=0; k<r; ++k )
            {
                T* data = &recvBuf[k*portionSize];

                const Int shift = Shift_(myCol+c*k,colAlignmentA,p);
                const Int offset = (shift-colShiftA) / c;
                const Int thisLocalHeight = Length_(height,shift,p);

                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    data[iLocal] = ABuffer[offset+iLocal*r];
            }
        }

        // A[VR,* ] <- A[MR,MC]
        mpi::Scatter
        ( recvBuf, portionSize, 
          sendBuf, portionSize, ownerRow, g.ColComm() );

        // A[VC,* ] <- A[VR,* ]
        mpi::SendRecv
        ( sendBuf, portionSize, sendRankCM, 0,
          recvBuf, portionSize, recvRankCM, mpi::ANY_TAG, g.VCComm() );

        // A[MC,MR] <- A[VC,* ]
        mpi::Gather
        ( recvBuf, portionSize, 
          sendBuf, portionSize, ownerCol, g.RowComm() );

        if( myCol == ownerCol )
        {
            // Unpack
            T* thisBuffer = this->Buffer();
#ifdef HAVE_OPENMP
            #pragma omp parallel for  
#endif
            for( Int k=0; k<c; ++k )
            {
                const T* data = &sendBuf[k*portionSize];

                const Int shift = Shift_(myRow+r*k,colAlignment,p);
                const Int offset = (shift-colShift) / r;
                const Int thisLocalHeight = Length_(height,shift,p);

                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    thisBuffer[offset+iLocal*c] = data[iLocal];
            }
        }
        this->auxMemory_.Release();
    }
    else if( A.Height() == 1 )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = g.Size();
        const Int myRow = g.Row();
        const Int myCol = g.Col();
        const Int rankCM = g.VCRank();
        const Int rankRM = g.VRRank();
        const Int ownerRow = this->ColAlignment();
        const Int ownerCol = A.ColAlignment();
        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentA = A.RowAlignment();
        const Int rowShift = this->RowShift();
        const Int rowShiftA = A.RowShift();

        const Int width = A.Width();
        const Int maxLocalWidth = MaxLength(width,p);

        const Int portionSize = std::max(maxLocalWidth,mpi::MIN_COLL_MSG);

        const Int rowShiftVR = Shift(rankRM,rowAlignment,p);
        const Int rowShiftVCA = Shift(rankCM,rowAlignmentA,p);
        const Int sendRankRM = (rankRM+(p+rowShiftVCA-rowShiftVR)) % p;
        const Int recvRankCM = (rankCM+(p+rowShiftVR-rowShiftVCA)) % p;
        const Int recvRankRM = (recvRankCM/r)+c*(recvRankCM%r);

        this->auxMemory_.Require( (r+c)*portionSize );
        T* buffer = this->auxMemory_.Buffer();
        T* sendBuf = &buffer[0];
        T* recvBuf = &buffer[r*portionSize];

        if( myCol == ownerCol )
        {
            // Pack
            const T* ABuffer = A.LockedBuffer();
            const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for  
#endif
            for( Int k=0; k<c; ++k )
            {
                T* data = &recvBuf[k*portionSize];

                const Int shift = Shift_(myRow+r*k,rowAlignmentA,p);
                const Int offset = (shift-rowShiftA) / r;
                const Int thisLocalWidth = Length_(width,shift,p);

                for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                    data[jLocal] = ABuffer[(offset+jLocal*c)*ALDim];
            }
        }

        // A[* ,VC] <- A[MR,MC]
        mpi::Scatter
        ( recvBuf, portionSize, 
          sendBuf, portionSize, ownerCol, g.RowComm() );

        // A[* ,VR] <- A[* ,VC]
        mpi::SendRecv
        ( sendBuf, portionSize, sendRankRM, 0,
          recvBuf, portionSize, recvRankRM, mpi::ANY_TAG, g.VRComm() );

        // A[MC,MR] <- A[* ,VR]
        mpi::Gather
        ( recvBuf, portionSize, 
          sendBuf, portionSize, ownerRow, g.ColComm() );
    
        if( myRow == ownerRow )
        {
            // Unpack
            T* thisBuffer = this->Buffer();
            const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for  
#endif
            for( Int k=0; k<r; ++k )
            {
                const T* data = &sendBuf[k*portionSize];

                const Int shift = Shift_(myCol+c*k,rowAlignment,p);
                const Int offset = (shift-rowShift) / c;
                const Int thisLocalWidth = Length_(width,shift,p);

                for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                    thisBuffer[(offset+jLocal*r)*thisLDim] = data[jLocal];
            }
        }

        this->auxMemory_.Release();
    }
    else
    {
        if( A.Height() >= A.Width() )
        {
            std::auto_ptr<DistMatrix<T,VR,STAR,Int> > A_VR_STAR
            ( new DistMatrix<T,VR,STAR,Int>(g) );

            *A_VR_STAR = A;

            std::auto_ptr<DistMatrix<T,VC,STAR,Int> > A_VC_STAR
            ( new DistMatrix<T,VC,STAR,Int>(true,this->ColAlignment(),g) );
            *A_VC_STAR = *A_VR_STAR;
            delete A_VR_STAR.release(); // lowers memory highwater

            *this = *A_VC_STAR;
        }
        else
        {
            std::auto_ptr<DistMatrix<T,STAR,VC,Int> > A_STAR_VC
            ( new DistMatrix<T,STAR,VC,Int>(g) );
            *A_STAR_VC = A;

            std::auto_ptr<DistMatrix<T,STAR,VR,Int> > A_STAR_VR
            ( new DistMatrix<T,STAR,VR,Int>(true,this->RowAlignment(),g) );
            *A_STAR_VR = *A_STAR_VC;
            delete A_STAR_VC.release(); // lowers memory highwater

            *this = *A_STAR_VR;
            this->ResizeTo( A_STAR_VR->Height(), A_STAR_VR->Width() );
        }
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,MR,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [MR,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();

    std::auto_ptr<DistMatrix<T,VR,STAR,Int> > A_VR_STAR
    ( new DistMatrix<T,VR,STAR,Int>(g) );
    *A_VR_STAR = A;

    std::auto_ptr<DistMatrix<T,VC,STAR,Int> > A_VC_STAR
    ( new DistMatrix<T,VC,STAR,Int>(true,this->ColAlignment(),g) );
    *A_VC_STAR = *A_VR_STAR;
    delete A_VR_STAR.release(); // lowers memory highwater

    *this = *A_VC_STAR;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,STAR,MC,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [* ,MC]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();

    std::auto_ptr<DistMatrix<T,STAR,VC,Int> > A_STAR_VC
    ( new DistMatrix<T,STAR,VC,Int>(g) );
    *A_STAR_VC = A;

    std::auto_ptr<DistMatrix<T,STAR,VR,Int> > A_STAR_VR
    ( new DistMatrix<T,STAR,VR,Int>(true,this->RowAlignment(),g) );
    *A_STAR_VR = *A_STAR_VC;
    delete A_STAR_VC.release(); // lowers memory highwater

    *this = *A_STAR_VR;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,VC,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [VC,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();

    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->colAlignment_ = A.ColAlignment() % g.Height();
            this->SetColShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return *this;

    if( this->ColAlignment() == A.ColAlignment() % g.Height() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = r * c;
        const Int row = g.Row();
        const Int colShift = this->ColShift();
        const Int rowAlignment = this->RowAlignment();
        const Int colAlignmentA = A.ColAlignment();

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localWidth = this->LocalWidth();
        const Int localHeightA = A.LocalHeight();

        const Int maxHeight = MaxLength(height,p);
        const Int maxWidth = MaxLength(width,c);
        const Int portionSize = std::max(maxHeight*maxWidth,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( 2*c*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[c*portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for  
#endif
        for( Int k=0; k<c; ++k )
        {
            T* data = &sendBuffer[k*portionSize];

            const Int thisRowShift = Shift_(k,rowAlignment,c);
            const Int thisLocalWidth = Length_(width,thisRowShift,c);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* ACol = &ABuffer[(thisRowShift+jLocal*c)*ALDim];
                T* dataCol = &data[jLocal*localHeightA];
                MemCopy( dataCol, ACol, localHeightA );
            }
        }

        // Communicate
        mpi::AllToAll
        ( sendBuffer, portionSize,
          recvBuffer, portionSize, g.RowComm() );

        // Unpack
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for 
#endif
        for( Int k=0; k<c; ++k )
        {
            const T* data = &recvBuffer[k*portionSize];

            const Int thisRank = row+k*r;
            const Int thisColShift = Shift_(thisRank,colAlignmentA,p);
            const Int thisColOffset = (thisColShift-colShift) / r;
            const Int thisLocalHeight = Length_(height,thisColShift,p);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &thisBuffer[thisColOffset+jLocal*thisLDim];
                const T* sourceCol = &data[jLocal*thisLocalHeight];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal*c] = sourceCol[iLocal];
            }
        }
        this->auxMemory_.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [MC,MR] <- [VC,* ]." << std::endl;
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = r * c;
        const Int row = g.Row();
        const Int colShift = this->ColShift();
        const Int colAlignment = this->ColAlignment();
        const Int rowAlignment = this->RowAlignment();
        const Int colAlignmentA = A.ColAlignment();

        const Int sendRow = (row+r+colAlignment-(colAlignmentA%r)) % r;
        const Int recvRow = (row+r+(colAlignmentA%r)-colAlignment) % r;

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localWidth = this->LocalWidth();
        const Int localHeightA = A.LocalHeight();

        const Int maxHeight = MaxLength(height,p);
        const Int maxWidth = MaxLength(width,c);
        const Int portionSize = std::max(maxHeight*maxWidth,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( 2*c*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[c*portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<c; ++k )
        {
            T* data = &secondBuffer[k*portionSize];

            const Int thisRowShift = Shift_(k,rowAlignment,c);
            const Int thisLocalWidth = Length_(width,thisRowShift,c);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* ACol = &ABuffer[(thisRowShift+jLocal*c)*ALDim];
                T* dataCol = &data[jLocal*localHeightA];
                MemCopy( dataCol, ACol, localHeightA );
            }
        }

        // SendRecv: properly align A[VC,*] via a trade in the column
        mpi::SendRecv
        ( secondBuffer, c*portionSize, sendRow, 0,
          firstBuffer,  c*portionSize, recvRow, 0, g.ColComm() );

        // AllToAll to gather all of the aligned A[VC,*] data into 
        // secondBuff.
        mpi::AllToAll
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.RowComm() );

        // Unpack
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<c; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const Int thisRank = recvRow+k*r;
            const Int thisColShift = Shift_(thisRank,colAlignmentA,p);
            const Int thisColOffset = (thisColShift-colShift) / r;
            const Int thisLocalHeight = Length_(height,thisColShift,p);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &thisBuffer[thisColOffset+jLocal*thisLDim];
                const T* sourceCol = &data[jLocal*thisLocalHeight];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal*c] = sourceCol[iLocal];
            }
        }
        this->auxMemory_.Release();
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,STAR,VC,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [* ,VC]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    DistMatrix<T,STAR,VR,Int> A_STAR_VR(true,this->RowAlignment(),g);

    A_STAR_VR = A;
    *this = A_STAR_VR;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,VR,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [VR,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    DistMatrix<T,VC,STAR,Int> A_VC_STAR(true,this->ColAlignment(),g);

    A_VC_STAR = A;
    *this = A_VC_STAR;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,STAR,VR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [* ,VR]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->rowAlignment_ = A.RowAlignment() % g.Width();
            this->SetRowShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return *this;

    if( this->RowAlignment() == A.RowAlignment() % g.Width() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = r * c;
        const Int col = g.Col();
        const Int rowShift = this->RowShift();
        const Int colAlignment = this->ColAlignment();
        const Int rowAlignmentA = A.RowAlignment();
    
        const Int height = this->Height();
        const Int width = this->Width();
        const Int localHeight = this->LocalHeight();
        const Int localWidthA = A.LocalWidth();

        const Int maxHeight = MaxLength(height,r);
        const Int maxWidth = MaxLength(width,p);
        const Int portionSize = std::max(maxHeight*maxWidth,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( 2*r*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[r*portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            T* data = &sendBuffer[k*portionSize];

            const Int thisColShift = Shift_(k,colAlignment,r);
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidthA; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = &ABuffer[thisColShift+jLocal*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }

        // Communicate
        mpi::AllToAll
        ( sendBuffer, portionSize,
          recvBuffer, portionSize, g.ColComm() );

        // Unpack
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            const T* data = &recvBuffer[k*portionSize];

            const Int thisRank = col+k*c;
            const Int thisRowShift = Shift_(thisRank,rowAlignmentA,p);
            const Int thisRowOffset = (thisRowShift-rowShift) / c;
            const Int thisLocalWidth = Length_(width,thisRowShift,p);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisBuffer[(thisRowOffset+jLocal*r)*thisLDim];
                MemCopy( thisCol, dataCol, localHeight );
            }
        }
        this->auxMemory_.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [MC,MR] <- [* ,VR]." << std::endl;
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = r * c;
        const Int col = g.Col();
        const Int rowShift = this->RowShift();
        const Int colAlignment = this->ColAlignment();
        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentA = A.RowAlignment();

        const Int sendCol = (col+c+rowAlignment-(rowAlignmentA%c)) % c;
        const Int recvCol = (col+c+(rowAlignmentA%c)-rowAlignment) % c;

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localHeight = this->LocalHeight();
        const Int localWidthA = A.LocalWidth();
            
        const Int maxHeight = MaxLength(height,r);
        const Int maxWidth = MaxLength(width,p);
        const Int portionSize = std::max(maxHeight*maxWidth,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( 2*r*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[r*portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            T* data = &secondBuffer[k*portionSize];

            const Int thisColShift = Shift_(k,colAlignment,r);
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidthA; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = &ABuffer[thisColShift+jLocal*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }

        // SendRecv: properly align A[*,VR] via a trade in the column
        mpi::SendRecv
        ( secondBuffer, r*portionSize, sendCol, 0,
          firstBuffer,  r*portionSize, recvCol, 0, g.RowComm() );

        // AllToAll to gather all of the aligned [*,VR] data into 
        // secondBuffer
        mpi::AllToAll
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.ColComm() );

        // Unpack
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const Int thisRank = recvCol+k*c;
            const Int thisRowShift = Shift_(thisRank,rowAlignmentA,p);
            const Int thisRowOffset = (thisRowShift-rowShift) / c;
            const Int thisLocalWidth = Length_(width,thisRowShift,p);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*localHeight];
                T* thisCol = &thisBuffer[(thisRowOffset+jLocal*r)*thisLDim];
                MemCopy( thisCol, dataCol, localHeight );
            }
        }
        this->auxMemory_.Release();
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,MC,MR,Int>&
DistMatrix<T,MC,MR,Int>::operator=( const DistMatrix<T,STAR,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR] = [* ,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );

    const Int r = this->Grid().Height();
    const Int c = this->Grid().Width();
    const Int colShift = this->ColShift();
    const Int rowShift = this->RowShift();

    const Int localHeight = this->LocalHeight();
    const Int localWidth = this->LocalWidth();

    const T* ABuffer = A.LockedBuffer();
    const Int ALDim = A.LDim();
    T* thisBuffer = this->Buffer();
    const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        T* destCol = &thisBuffer[jLocal*thisLDim];
        const T* sourceCol = &ABuffer[colShift+(rowShift+jLocal*c)*ALDim];
        for( Int iLocal=0; iLocal<localHeight; ++iLocal )
            destCol[iLocal] = sourceCol[iLocal*r];
    }
    return *this;
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SumScatterFrom( const DistMatrix<T,MC,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SumScatterFrom([MC,* ])");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedColAlignment() )
        {
            this->colAlignment_ = A.ColAlignment();
            this->SetColShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return;

    if( this->ColAlignment() == A.ColAlignment() )
    {
        if( this->Width() == 1 )
        {
            const Int rowAlignment = this->RowAlignment();
            const Int myCol = g.Col();

            const Int localHeight = this->LocalHeight();

            const Int recvSize = std::max(localHeight,mpi::MIN_COLL_MSG);
            const Int sendSize = recvSize;

            this->auxMemory_.Require( sendSize + recvSize );

            T* buffer = this->auxMemory_.Buffer();
            T* sendBuffer = &buffer[0];
            T* recvBuffer = &buffer[sendSize];

            // Pack 
            const T* ACol = A.LockedBuffer();
            MemCopy( sendBuffer, ACol, localHeight );

            // Reduce to rowAlignment
            mpi::Reduce
            ( sendBuffer, recvBuffer, sendSize, 
              mpi::SUM, rowAlignment, g.RowComm() );

            if( myCol == rowAlignment )
            {
                T* thisCol = this->Buffer();
                MemCopy( thisCol, recvBuffer, localHeight );
            }

            this->auxMemory_.Release();
        }
        else
        {
            const Int c = g.Width();
            const Int rowAlignment = this->RowAlignment();
            
            const Int width = this->Width();
            const Int localHeight = this->LocalHeight();
            const Int localWidth = this->LocalWidth();
            const Int maxLocalWidth = MaxLength(width,c);

            const Int recvSize = 
                std::max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);
            const Int sendSize = c * recvSize;

            this->auxMemory_.Require( sendSize );
            T* buffer = this->auxMemory_.Buffer();
            
            // Pack 
            const T* ABuffer = A.LockedBuffer();
            const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int k=0; k<c; ++k )
            {
                T* data = &buffer[k*recvSize];

                const Int thisRowShift = Shift_( k, rowAlignment, c );
                const Int thisLocalWidth = Length_(width,thisRowShift,c);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
                #pragma omp parallel for
#endif
                for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                {
                    const T* ACol = &ABuffer[(thisRowShift+jLocal*c)*ALDim];
                    T* dataCol = &data[jLocal*localHeight];
                    MemCopy( dataCol, ACol, localHeight );
                }
            }

            // Communicate
            mpi::ReduceScatter( buffer, recvSize, mpi::SUM, g.RowComm() );

            // Unpack our received data
            T* thisBuffer = this->Buffer();
            const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* bufferCol = &buffer[jLocal*localHeight];
                T* thisCol = &thisBuffer[jLocal*thisLDim];
                MemCopy( thisCol, bufferCol, localHeight );
            }
            this->auxMemory_.Release();
        }
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned SumScatterFrom [MC,MR] <- [MC,* ]." 
                      << std::endl;
#endif
        if( this->Width() == 1 )
        {
            const Int r = g.Height();
            const Int rowAlignment = this->RowAlignment();
            const Int myRow = g.Row();
            const Int myCol = g.Col();

            const Int height = this->Height();
            const Int localHeight = this->LocalHeight();
            const Int localHeightA = A.LocalHeight();
            const Int maxLocalHeight = MaxLength(height,r);

            const Int portionSize = std::max(maxLocalHeight,mpi::MIN_COLL_MSG);

            const Int colAlignment = this->ColAlignment();
            const Int colAlignmentA = A.ColAlignment();
            const Int sendRow = (myRow+r+colAlignment-colAlignmentA) % r;
            const Int recvRow = (myRow+r+colAlignmentA-colAlignment) % r;

            this->auxMemory_.Require( 2*portionSize );

            T* buffer = this->auxMemory_.Buffer();
            T* sendBuffer = &buffer[0];
            T* recvBuffer = &buffer[portionSize];

            // Pack 
            const T* ACol = A.LockedBuffer();
            MemCopy( sendBuffer, ACol, localHeightA );
            
            // Reduce to rowAlignment
            mpi::Reduce
            ( sendBuffer, recvBuffer, portionSize, 
              mpi::SUM, rowAlignment, g.RowComm() );

            if( myCol == rowAlignment )
            {
                // Perform the realignment
                mpi::SendRecv
                ( recvBuffer, portionSize, sendRow, 0,
                  sendBuffer, portionSize, recvRow, 0, g.ColComm() );

                T* thisCol = this->Buffer();
                MemCopy( thisCol, sendBuffer, localHeight );
            }

            this->auxMemory_.Release();
        }
        else
        {
            const Int r = g.Height();
            const Int c = g.Width();
            const Int row = g.Row();

            const Int colAlignment = this->ColAlignment();
            const Int rowAlignment = this->RowAlignment();
            const Int colAlignmentA = A.ColAlignment();
            const Int sendRow = (row+r+colAlignment-colAlignmentA) % r;
            const Int recvRow = (row+r+colAlignmentA-colAlignment) % r;

            const Int width = this->Width();
            const Int localHeight = this->LocalHeight();
            const Int localWidth = this->LocalWidth();
            const Int localHeightA = A.LocalHeight();
            const Int maxLocalWidth = MaxLength(width,c);

            const Int recvSize_RS = 
                std::max(localHeightA*maxLocalWidth,mpi::MIN_COLL_MSG);
            const Int sendSize_RS = c * recvSize_RS;
            const Int recvSize_SR = localHeight * localWidth;

            this->auxMemory_.Require
            ( recvSize_RS + std::max(sendSize_RS,recvSize_SR) );

            T* buffer = this->auxMemory_.Buffer();
            T* firstBuffer = &buffer[0];
            T* secondBuffer = &buffer[recvSize_RS];

            // Pack 
            // TODO: Stick an optional outer parallelization here?
            const T* ABuffer = A.LockedBuffer();
            const Int ALDim = A.LDim();
            for( Int k=0; k<c; ++k )
            {
                T* data = &secondBuffer[k*recvSize_RS];

                const Int thisRowShift = Shift_( k, rowAlignment, c );
                const Int thisLocalWidth = Length_(width,thisRowShift,c);

#ifdef HAVE_OPENMP
                #pragma omp parallel for
#endif
                for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                {
                    const Int j = thisRowShift + jLocal*c;
                    const T* ACol = &ABuffer[j*ALDim];
                    T* dataCol = &data[jLocal*localHeightA];
                    MemCopy( dataCol, ACol, localHeightA );
                }
            }

            // Reduce-scatter over each process row
            mpi::ReduceScatter
            ( secondBuffer, firstBuffer, recvSize_RS, mpi::SUM, g.RowComm() );

            // Trade reduced data with the appropriate process row
            mpi::SendRecv
            ( firstBuffer,  localHeightA*localWidth, sendRow, 0,
              secondBuffer, localHeight*localWidth,    recvRow, 0, 
              g.ColComm() );

            // Unpack the received data
            T* thisBuffer = this->Buffer();
            const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* secondBufferCol = &secondBuffer[jLocal*localHeight];
                T* thisCol = &thisBuffer[jLocal*thisLDim];
                MemCopy( thisCol, secondBufferCol, localHeight );
            }
            this->auxMemory_.Release();
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SumScatterFrom( const DistMatrix<T,STAR,MR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SumScatterFrom([* ,MR])");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
#ifdef VECTOR_WARNINGS
    if( A.Width() == 1 && g.Rank() == 0 )
    {
        std::cerr <<
          "The vector version of [MC,MR].SumScatterFrom([* ,MR]) does not "
          "yet have a vector version implemented, but it would only require"
          " a modification of the vector version of "
          "[MC,MR].SumScatterFrom([MC,* ])" << std::endl;
    }
#endif
#ifdef CACHE_WARNINGS
    if( A.Width() != 1 && g.Rank() == 0 )
    {
        std::cerr << 
          "[MC,MR]::SumScatterFrom([* ,MR]) potentially causes a large "
          "amount of cache-thrashing. If possible, avoid it by forming the "
          "(conjugate-)transpose of the [* ,MR] matrix instead." << std::endl;
    }
#endif
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->rowAlignment_ = A.RowAlignment();
            this->SetRowShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return;

    if( this->RowAlignment() == A.RowAlignment() )
    {
        const Int r = g.Height();
        const Int colAlignment = this->ColAlignment();

        const Int height = this->Height();
        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const Int maxLocalHeight = MaxLength(height,r);

        const Int recvSize = 
            std::max(maxLocalHeight*localWidth,mpi::MIN_COLL_MSG);
        const Int sendSize = r * recvSize;

        this->auxMemory_.Require( sendSize );
        T* buffer = this->auxMemory_.Buffer();

        // Pack 
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            T* data = &buffer[k*recvSize];

            const Int thisColShift = Shift_(k,colAlignment,r);
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = &ABuffer[thisColShift+jLocal*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }

        // Communicate
        mpi::ReduceScatter( buffer, recvSize, mpi::SUM, g.ColComm() );

        // Unpack our received data
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* bufferCol = &buffer[jLocal*localHeight];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            MemCopy( thisCol, bufferCol, localHeight );
        }
        this->auxMemory_.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned SumScatterFrom [MC,MR] <- [* ,MR]." 
                      << std::endl;
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int col = g.Col();

        const Int colAlignment = this->ColAlignment();
        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentA = A.RowAlignment();
        const Int sendCol = (col+c+rowAlignment-rowAlignmentA) % c;
        const Int recvCol = (col+c+rowAlignmentA-rowAlignment) % c;

        const Int height = this->Height();
        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const Int localWidthA = A.LocalWidth();
        const Int maxLocalHeight = MaxLength(height,r);

        const Int recvSize_RS = 
            std::max(maxLocalHeight*localWidthA,mpi::MIN_COLL_MSG);
        const Int sendSize_RS = r * recvSize_RS;
        const Int recvSize_SR = localHeight * localWidth;

        this->auxMemory_.Require
        ( recvSize_RS + std::max(sendSize_RS,recvSize_SR) );

        T* buffer = this->auxMemory_.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[recvSize_RS];

        // Pack 
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            T* data = &secondBuffer[k*recvSize_RS];

            const Int thisColShift = Shift_(k,colAlignment,r);
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localWidthA; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = &ABuffer[thisColShift+jLocal*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }

        // Reduce-scatter over each process col
        mpi::ReduceScatter
        ( secondBuffer, firstBuffer, recvSize_RS, mpi::SUM, g.ColComm() );

        // Trade reduced data with the appropriate process col
        mpi::SendRecv
        ( firstBuffer,  localHeight*localWidthA, sendCol, 0,
          secondBuffer, localHeight*localWidth,    recvCol, 0, g.RowComm() );

        // Unpack the received data
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* secondBufferCol = &secondBuffer[jLocal*localHeight];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            MemCopy( thisCol, secondBufferCol, localHeight );
        }
        this->auxMemory_.Release();
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SumScatterFrom( const DistMatrix<T,STAR,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SumScatterFrom([* ,* ])");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );

    const elem::Grid& g = this->Grid();
    if( !g.InGrid() )
        return;

    const Int r = g.Height();
    const Int c = g.Width();
    const Int colAlignment = this->ColAlignment();
    const Int rowAlignment = this->RowAlignment();

    const Int height = this->Height();
    const Int width = this->Width();
    const Int localHeight = this->LocalHeight();
    const Int localWidth = this->LocalWidth();
    const Int maxLocalHeight = MaxLength(height,r);
    const Int maxLocalWidth = MaxLength(width,c);

    const Int recvSize = 
        std::max(maxLocalHeight*maxLocalWidth,mpi::MIN_COLL_MSG);
    const Int sendSize = r*c*recvSize;

    this->auxMemory_.Require( sendSize );
    T* buffer = this->auxMemory_.Buffer();

    // Pack 
    const T* ABuffer = A.LockedBuffer();
    const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
    #pragma omp parallel for
#endif
    for( Int l=0; l<c; ++l )
    {
        const Int thisRowShift = Shift_( l, rowAlignment, c );
        const Int thisLocalWidth = Length_( width, thisRowShift, c );

        for( Int k=0; k<r; ++k )
        {
            T* data = &buffer[(k+l*r)*recvSize];

            const Int thisColShift = Shift_(k,colAlignment,r);
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = 
                    &ABuffer[thisColShift+(thisRowShift+jLocal*c)*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }
    }

    // Communicate
    mpi::ReduceScatter( buffer, recvSize, mpi::SUM, g.VCComm() );

    // Unpack our received data
    T* thisBuffer = this->Buffer();
    const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const T* bufferCol = &buffer[jLocal*localHeight];
        T* thisCol = &thisBuffer[jLocal*thisLDim];
        MemCopy( thisCol, bufferCol, localHeight );
    }
    this->auxMemory_.Release();
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SumScatterUpdate
( T alpha, const DistMatrix<T,MC,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SumScatterUpdate([MC,* ])");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    if( !g.InGrid() )
        return;

    if( this->ColAlignment() == A.ColAlignment() )
    {
        if( this->Width() == 1 )
        {
            const Int rowAlignment = this->RowAlignment();
            const Int myCol = g.Col();

            const Int localHeight = this->LocalHeight();

            const Int portionSize = std::max(localHeight,mpi::MIN_COLL_MSG);

            this->auxMemory_.Require( 2*portionSize );

            T* buffer = this->auxMemory_.Buffer();
            T* sendBuffer = &buffer[0];
            T* recvBuffer = &buffer[portionSize];

            // Pack 
            const T* ACol = A.LockedBuffer(0,0);
            MemCopy( sendBuffer, ACol, localHeight );
            
            // Reduce to rowAlignment
            mpi::Reduce
            ( sendBuffer, recvBuffer, portionSize, 
              mpi::SUM, rowAlignment, g.RowComm() );

            if( myCol == rowAlignment )
            {
                T* thisCol = this->Buffer(0,0);
#if defined(HAVE_OPENMP) && !defined(AVOID_OMP_FMA)
                #pragma omp parallel for
#endif
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    thisCol[iLocal] += alpha*recvBuffer[iLocal];
            }

            this->auxMemory_.Release();
        }
        else
        {
            const Int c = g.Width();
            const Int rowAlignment = this->RowAlignment();

            const Int width = this->Width();
            const Int localHeight = this->LocalHeight();
            const Int localWidth = this->LocalWidth();
            const Int maxLocalWidth = MaxLength(width,c);

            const Int portionSize = 
                std::max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);
            const Int sendSize = c*portionSize;

            this->auxMemory_.Require( sendSize );
            T* buffer = this->auxMemory_.Buffer();

            // Pack 
            const T* ABuffer = A.LockedBuffer();
            const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int k=0; k<c; ++k )
            {
                T* data = &buffer[k*portionSize];

                const Int thisRowShift = Shift_( k, rowAlignment, c );
                const Int thisLocalWidth = Length_(width,thisRowShift,c);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
                #pragma omp parallel for
#endif
                for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                {
                    const T* ACol = &ABuffer[(thisRowShift+jLocal*c)*ALDim];
                    T* dataCol = &data[jLocal*localHeight];
                    MemCopy( dataCol, ACol, localHeight );
                }
            }
            
            // Communicate
            mpi::ReduceScatter( buffer, portionSize, mpi::SUM, g.RowComm() );

            // Update with our received data
            T* thisBuffer = this->Buffer();
            const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* bufferCol = &buffer[jLocal*localHeight];
                T* thisCol = &thisBuffer[jLocal*thisLDim];
                blas::Axpy( localHeight, alpha, bufferCol, 1, thisCol, 1 );
            }
            this->auxMemory_.Release();
        }
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
        {
            std::cerr << "Unaligned SumScatterUpdate [MC,MR] <- [MC,* ]." 
                      << std::endl;
        }
#endif
        if( this->Width() == 1 )
        {
            const Int r = g.Height();
            const Int rowAlignment = this->RowAlignment();
            const Int myRow = g.Row();
            const Int myCol = g.Col();

            const Int height = this->Height();
            const Int localHeight = this->LocalHeight();
            const Int localHeightA = A.LocalHeight();
            const Int maxLocalHeight = MaxLength(height,r);

            const Int portionSize = std::max(maxLocalHeight,mpi::MIN_COLL_MSG);

            const Int colAlignment = this->ColAlignment();
            const Int colAlignmentA = A.ColAlignment();
            const Int sendRow = (myRow+r+colAlignment-colAlignmentA) % r;
            const Int recvRow = (myRow+r+colAlignmentA-colAlignment) % r;

            this->auxMemory_.Require( 2*portionSize );

            T* buffer = this->auxMemory_.Buffer();
            T* sendBuffer = &buffer[0];
            T* recvBuffer = &buffer[portionSize];

            // Pack 
            const T* ACol = A.LockedBuffer();
            MemCopy( sendBuffer, ACol, localHeightA );
            
            // Reduce to rowAlignment
            mpi::Reduce
            ( sendBuffer, recvBuffer, portionSize, 
              mpi::SUM, rowAlignment, g.RowComm() );

            if( myCol == rowAlignment )
            {
                // Perform the realignment
                mpi::SendRecv
                ( recvBuffer, portionSize, sendRow, 0,
                  sendBuffer, portionSize, recvRow, 0, g.ColComm() );

                T* thisCol = this->Buffer(0,0);
#if defined(HAVE_OPENMP) && !defined(AVOID_OMP_FMA)
                #pragma omp parallel for
#endif
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    thisCol[iLocal] += alpha*sendBuffer[iLocal];
            }
            this->auxMemory_.Release();
        }
        else
        {
            const Int r = g.Height();
            const Int c = g.Width();
            const Int row = g.Row();

            const Int colAlignment = this->ColAlignment();
            const Int rowAlignment = this->RowAlignment();
            const Int colAlignmentA = A.ColAlignment();
            const Int sendRow = (row+r+colAlignment-colAlignmentA) % r;
            const Int recvRow = (row+r+colAlignmentA-colAlignment) % r;

            const Int width = this->Width();
            const Int localHeight = this->LocalHeight();
            const Int localWidth = this->LocalWidth();
            const Int localHeightA = A.LocalHeight();
            const Int maxLocalWidth = MaxLength(width,c);

            const Int recvSize_RS = 
                std::max(localHeightA*maxLocalWidth,mpi::MIN_COLL_MSG);
            const Int sendSize_RS = c * recvSize_RS;
            const Int recvSize_SR = localHeight * localWidth;

            this->auxMemory_.Require
            ( recvSize_RS + std::max(sendSize_RS,recvSize_SR) );

            T* buffer = this->auxMemory_.Buffer();
            T* firstBuffer = &buffer[0];
            T* secondBuffer = &buffer[recvSize_RS];

            // Pack 
            const T* ABuffer = A.LockedBuffer();
            const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int k=0; k<c; ++k )
            {
                T* data = &secondBuffer[k*recvSize_RS];

                const Int thisRowShift = Shift_( k, rowAlignment, c );
                const Int thisLocalWidth = Length_(width,thisRowShift,c);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
                #pragma omp parallel for
#endif
                for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                {
                    const T* ACol = &ABuffer[(thisRowShift+jLocal*c)*ALDim];
                    T* dataCol = &data[jLocal*localHeightA];
                    MemCopy( dataCol, ACol, localHeightA );
                }
            }

            // Reduce-scatter over each process row
            mpi::ReduceScatter
            ( secondBuffer, firstBuffer, recvSize_RS, mpi::SUM, g.RowComm() );

            // Trade reduced data with the appropriate process row
            mpi::SendRecv
            ( firstBuffer,  localHeightA*localWidth, sendRow, 0,
              secondBuffer, localHeight*localWidth,    recvRow, 0, 
              g.ColComm() );

            // Update with our received data
            T* thisBuffer = this->Buffer();
            const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(AVOID_OMP_FMA)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* secondBufferCol = &secondBuffer[jLocal*localHeight];
                T* thisCol = &thisBuffer[jLocal*thisLDim];
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    thisCol[iLocal] += alpha*secondBufferCol[iLocal];
            }
            this->auxMemory_.Release();
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SumScatterUpdate
( T alpha, const DistMatrix<T,STAR,MR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SumScatterUpdate([* ,MR])");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
#ifdef VECTOR_WARNINGS
    if( A.Width() == 1 && g.Rank() == 0 )
    {
        std::cerr <<
          "The vector version of [MC,MR].SumScatterUpdate([* ,MR]) does not"
          " yet have a vector version implemented, but it would only "
          "require a modification of the vector version of "
          "[MC,MR].SumScatterUpdate([MC,* ])" << std::endl;
    }
#endif
#ifdef CACHE_WARNINGS
    if( A.Width() != 1 && g.Rank() == 0 )
    {
        std::cerr << 
          "[MC,MR]::SumScatterUpdate([* ,MR]) potentially causes a large "
          "amount of cache-thrashing. If possible, avoid it by forming the "
          "(conjugate-)transpose of the [* ,MR] matrix instead." << std::endl;
    }
#endif
    if( !g.InGrid() )
        return;

    if( this->RowAlignment() == A.RowAlignment() )
    {
        const Int r = g.Height();
        const Int colAlignment = this->ColAlignment();

        const Int height = this->Height();
        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const Int maxLocalHeight = MaxLength(height,r);

        const Int recvSize = 
            std::max(maxLocalHeight*localWidth,mpi::MIN_COLL_MSG);
        const Int sendSize = r*recvSize;

        this->auxMemory_.Require( sendSize );
        T* buffer = this->auxMemory_.Buffer();

        // Pack 
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            T* data = &buffer[k*recvSize];

            const Int thisColShift = Shift_( k, colAlignment, r );
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = &ABuffer[thisColShift+jLocal*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }

        // Communicate
        mpi::ReduceScatter( buffer, recvSize, mpi::SUM, g.ColComm() );

        // Update with our received data
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(AVOID_OMP_FMA)
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* bufferCol = &buffer[jLocal*localHeight];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                thisCol[iLocal] += alpha*bufferCol[iLocal];
        }
        this->auxMemory_.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
        {
            std::cerr << "Unaligned SumScatterUpdate [MC,MR] <- [* ,MR]." 
                      << std::endl;
        }
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int col = g.Col();

        const Int colAlignment = this->ColAlignment();
        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentA = A.RowAlignment();
        const Int sendCol = (col+c+rowAlignment-rowAlignmentA) % c;
        const Int recvCol = (col+c+rowAlignmentA-rowAlignment) % c;

        const Int height = this->Height();
        const Int localHeight = this->LocalHeight();
        const Int localWidth = this->LocalWidth();
        const Int localWidthA = A.LocalWidth();
        const Int maxLocalHeight = MaxLength(height,r);

        const Int recvSize_RS = 
            std::max(maxLocalHeight*localWidthA,mpi::MIN_COLL_MSG);
        const Int sendSize_RS = r * recvSize_RS;
        const Int recvSize_SR = localHeight * localWidth;

        this->auxMemory_.Require
        ( recvSize_RS + std::max(sendSize_RS,recvSize_SR) );

        T* buffer = this->auxMemory_.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[recvSize_RS];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<r; ++k )
        {
            T* data = &secondBuffer[k*recvSize_RS];

            const Int thisColShift = Shift_( k, colAlignment, r );
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localWidthA; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = &ABuffer[thisColShift+jLocal*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }

        // Reduce-scatter over each process col
        mpi::ReduceScatter
        ( secondBuffer, firstBuffer, recvSize_RS, mpi::SUM, g.ColComm() );

        // Trade reduced data with the appropriate process col
        mpi::SendRecv
        ( firstBuffer,  localHeight*localWidthA, sendCol, 0,
          secondBuffer, localHeight*localWidth,    recvCol, mpi::ANY_TAG,
          g.RowComm() );

        // Update with our received data
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(AVOID_OMP_FMA)
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* secondBufferCol = &secondBuffer[jLocal*localHeight];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                thisCol[iLocal] += alpha*secondBufferCol[iLocal];
        }
        this->auxMemory_.Release();
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SumScatterUpdate
( T alpha, const DistMatrix<T,STAR,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SumScatterUpdate([* ,* ])");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );

    const elem::Grid& g = this->Grid();
    if( !g.InGrid() )
        return;

    const Int r = g.Height();
    const Int c = g.Width();
    const Int colAlignment = this->ColAlignment();
    const Int rowAlignment = this->RowAlignment();

    const Int height = this->Height();
    const Int width = this->Width();
    const Int localHeight = this->LocalHeight();
    const Int localWidth = this->LocalWidth();
    const Int maxLocalHeight = MaxLength(height,r);
    const Int maxLocalWidth = MaxLength(width,c);

    const Int recvSize = 
        std::max(maxLocalHeight*maxLocalWidth,mpi::MIN_COLL_MSG);
    const Int sendSize = r * c * recvSize;

    this->auxMemory_.Require( sendSize );
    T* buffer = this->auxMemory_.Buffer();

    // Pack 
    const T* ABuffer = A.LockedBuffer();
    const Int ALDim = A.LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
    #pragma omp parallel for
#endif
    for( Int l=0; l<c; ++l )
    {
        const Int thisRowShift = Shift_( l, rowAlignment, c );
        const Int thisLocalWidth = Length_( width, thisRowShift, c );

        for( Int k=0; k<r; ++k )
        {
            T* data = &buffer[(k+l*r)*recvSize];

            const Int thisColShift = Shift_( k, colAlignment, r );
            const Int thisLocalHeight = Length_(height,thisColShift,r);

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                T* destCol = &data[jLocal*thisLocalHeight];
                const T* sourceCol = 
                    &ABuffer[thisColShift+(thisRowShift+jLocal*c)*ALDim];
                for( Int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    destCol[iLocal] = sourceCol[iLocal*r];
            }
        }
    }

    // Communicate
    mpi::ReduceScatter( buffer, recvSize, mpi::SUM, g.VCComm() );

    // Unpack our received data
    T* thisBuffer = this->Buffer();
    const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(AVOID_OMP_FMA)
    #pragma omp parallel for
#endif
    for( Int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const T* bufferCol = &buffer[jLocal*localHeight];
        T* thisCol = &thisBuffer[jLocal*thisLDim];
        for( Int iLocal=0; iLocal<localHeight; ++iLocal )
            thisCol[iLocal] += alpha*bufferCol[iLocal];
    }
    this->auxMemory_.Release();
}

//
// Functions which explicitly work in the complex plane
//

template<typename T,typename Int>
BASE(T)
DistMatrix<T,MC,MR,Int>::GetRealPart( Int i, Int j ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetRealPart");
    this->AssertValidEntry( i, j );
#endif
    typedef BASE(T) R; 

    // We will determine the owner of the (i,j) entry and have him Broadcast
    // throughout the entire process grid
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (i + this->ColAlignment()) % g.Height();
    const Int ownerCol = (j + this->RowAlignment()) % g.Width();
    const Int ownerRank = ownerRow + ownerCol * g.Height();

    R u;
    if( g.VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / g.Height();
        const Int jLocal = (j-this->RowShift()) / g.Width();
        u = this->GetLocalRealPart(iLocal,jLocal);
    }
    mpi::Broadcast( &u, 1, g.VCToViewingMap(ownerRank), g.ViewingComm() );
    return u;
}

template<typename T,typename Int>
BASE(T)
DistMatrix<T,MC,MR,Int>::GetImagPart( Int i, Int j ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetImagPart");
    this->AssertValidEntry( i, j );
#endif
    typedef BASE(T) R; 

    // We will determine the owner of the (i,j) entry and have him Broadcast
    // throughout the entire process grid
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (i + this->ColAlignment()) % g.Height();
    const Int ownerCol = (j + this->RowAlignment()) % g.Width();
    const Int ownerRank = ownerRow + ownerCol * g.Height();

    R u;
    if( g.VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / g.Height();
        const Int jLocal = (j-this->RowShift()) / g.Width();
        u = this->GetLocalImagPart(iLocal,jLocal);
    }
    mpi::Broadcast( &u, 1, g.VCToViewingMap(ownerRank), g.ViewingComm() );
    return u;
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetRealPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetRealPart");
    this->AssertValidEntry( i, j );
#endif
    const elem::Grid& g = this->Grid(); 
    const Int ownerRow = (i + this->ColAlignment()) % g.Height();
    const Int ownerCol = (j + this->RowAlignment()) % g.Width();
    const Int ownerRank = ownerRow + ownerCol*g.Height();
    if( g.VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / g.Height();
        const Int jLocal = (j-this->RowShift()) / g.Width();
        this->SetLocalRealPart( iLocal, jLocal, u );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetImagPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetImagPart");
    this->AssertValidEntry( i, j );
#endif
    if( !IsComplex<T>::val )
        throw std::logic_error("Called complex-only routine with real data");
    
    const elem::Grid& g = this->Grid(); 
    const Int ownerRow = (i + this->ColAlignment()) % g.Height();
    const Int ownerCol = (j + this->RowAlignment()) % g.Width();
    const Int ownerRank = ownerRow + ownerCol*g.Height();
    if( g.VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / g.Height();
        const Int jLocal = (j-this->RowShift()) / g.Width();
        this->SetLocalRealPart( iLocal, jLocal, u );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::UpdateRealPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::UpdateRealPart");
    this->AssertValidEntry( i, j );
#endif
    const elem::Grid& g = this->Grid(); 
    const Int ownerRow = (i + this->ColAlignment()) % g.Height();
    const Int ownerCol = (j + this->RowAlignment()) % g.Width();
    const Int ownerRank = ownerRow + ownerCol*g.Height();
    if( g.VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / g.Height();
        const Int jLocal = (j-this->RowShift()) / g.Width();
        this->UpdateLocalRealPart( iLocal, jLocal, u );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::UpdateImagPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::UpdateImagPart");
    this->AssertValidEntry( i, j );
#endif
    if( !IsComplex<T>::val )
        throw std::logic_error("Called complex-only routine with real data");
    
    const elem::Grid& g = this->Grid(); 
    const Int ownerRow = (i + this->ColAlignment()) % g.Height();
    const Int ownerCol = (j + this->RowAlignment()) % g.Width();
    const Int ownerRank = ownerRow + ownerCol*g.Height();
    if( g.VCRank() == ownerRank )
    {
        const Int iLocal = (i-this->ColShift()) / g.Height();
        const Int jLocal = (j-this->RowShift()) / g.Width();
        this->UpdateLocalRealPart( iLocal, jLocal, u );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::GetRealPartOfDiagonal
( DistMatrix<BASE(T),MD,STAR,Int>& d, Int offset ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetRealPartOfDiagonal");
    if( d.Viewing() )
        this->AssertSameGrid( d.Grid() );
#endif
    const Int length = this->DiagonalLength( offset );
#ifndef RELEASE
    if( d.Viewing() && length != d.Height() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;

    const elem::Grid& g = this->Grid();
    if( !d.Viewing() )
    {
        d.SetGrid( g );
        if( !d.ConstrainedColAlignment() )
            d.AlignWithDiagonal( this->DistData(), offset );
        d.ResizeTo( length, 1 );
    }

    if( d.Participating() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.ColShift();

        Int iStart, jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalHeight();

        const T* thisBuffer = this->LockedBuffer();
        const Int thisLDim = this->LDim();
        R* dBuffer = d.Buffer();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            dBuffer[k] = RealPart(thisBuffer[iLocal+jLocal*thisLDim]);
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::GetImagPartOfDiagonal
( DistMatrix<BASE(T),MD,STAR,Int>& d, Int offset ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetImagPartOfDiagonal");
    if( d.Viewing() )
        this->AssertSameGrid( d.Grid() );
#endif
    const Int length = this->DiagonalLength( offset );
#ifndef RELEASE
    if( d.Viewing() && length != d.Height() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;

    const elem::Grid& g = this->Grid();
    if( !d.Viewing() )
    {
        d.SetGrid( g );
        if( !d.ConstrainedColAlignment() )
            d.AlignWithDiagonal( this->DistData(), offset );
        d.ResizeTo( length, 1 );
    }

    if( d.Participating() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.ColShift();

        Int iStart, jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalHeight();

        const T* thisBuffer = this->LockedBuffer();
        const Int thisLDim = this->LDim();
        R* dBuffer = d.Buffer();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            dBuffer[k] = ImagPart(thisBuffer[iLocal+jLocal*thisLDim]);
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::GetRealPartOfDiagonal
( DistMatrix<BASE(T),STAR,MD,Int>& d, Int offset ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetRealPartOfDiagonal");
    if( d.Viewing() )
        this->AssertSameGrid( d.Grid() );
#endif
    const Int length = this->DiagonalLength( offset );
#ifndef RELEASE
    if( d.Viewing() && length != d.Width() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;

    const elem::Grid& g = this->Grid();
    if( !d.Viewing() )
    {
        d.SetGrid( g );
        if( !d.ConstrainedRowAlignment() )
            d.AlignWithDiagonal( this->DistData(), offset );
        d.ResizeTo( 1, length );
    }

    if( d.Participating() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.RowShift();

        Int iStart, jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalWidth();

        const T* thisBuffer = this->LockedBuffer();
        const Int thisLDim = this->LDim();
        R* dBuffer = d.Buffer();
        const Int dLDim = d.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            dBuffer[k*dLDim] = RealPart(thisBuffer[iLocal+jLocal*thisLDim]);
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::GetImagPartOfDiagonal
( DistMatrix<BASE(T),STAR,MD,Int>& d, Int offset ) const
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::GetImagPartOfDiagonal");
    if( d.Viewing() )
        this->AssertSameGrid( d.Grid() );
#endif
    const Int length = this->DiagonalLength( offset );
#ifndef RELEASE
    if( d.Viewing() && length != d.Width() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;

    const elem::Grid& g = this->Grid();
    if( !d.Viewing() )
    {
        d.SetGrid( g );
        if( !d.ConstrainedRowAlignment() )
            d.AlignWithDiagonal( this->DistData(), offset );
        d.ResizeTo( 1, length );
    }

    if( d.Participating() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.RowShift();

        Int iStart, jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalWidth();

        const T* thisBuffer = this->LockedBuffer();
        const Int thisLDim = this->LDim();
        R* dBuffer = d.Buffer();
        const Int dLDim = d.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            dBuffer[k*dLDim] = ImagPart(thisBuffer[iLocal+jLocal*thisLDim]);
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetRealPartOfDiagonal
( const DistMatrix<BASE(T),MD,STAR,Int>& d, Int offset )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetRealPartOfDiagonal");
    this->AssertSameGrid( d.Grid() );
    if( d.Width() != 1 )
        throw std::logic_error("d must be a column vector");
    const Int length = this->DiagonalLength( offset );
    if( length != d.Height() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;

    if( d.Participating() )
    {
        const elem::Grid& g = this->Grid();
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.ColShift();

        Int iStart,jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalHeight();
        const R* dBuffer = d.LockedBuffer();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            this->SetLocalRealPart( iLocal, jLocal, dBuffer[k] );
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetImagPartOfDiagonal
( const DistMatrix<BASE(T),MD,STAR,Int>& d, Int offset )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetImagPartOfDiagonal");
    this->AssertSameGrid( d.Grid() );
    if( d.Width() != 1 )
        throw std::logic_error("d must be a column vector");
    const Int length = this->DiagonalLength( offset );
    if( length != d.Height() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;
    if( !IsComplex<T>::val )
        throw std::logic_error("Called complex-only routine with real data");

    if( d.Participating() )
    {
        const elem::Grid& g = this->Grid();
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.ColShift();

        Int iStart,jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalHeight();
        const R* dBuffer = d.LockedBuffer();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            this->SetLocalImagPart( iLocal, jLocal, dBuffer[k] );
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetRealPartOfDiagonal
( const DistMatrix<BASE(T),STAR,MD,Int>& d, Int offset )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetRealPartOfDiagonal");
    this->AssertSameGrid( d.Grid() );
    if( d.Height() != 1 )
        throw std::logic_error("d must be a row vector");
    const Int length = this->DiagonalLength( offset );
    if( length != d.Width() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;

    if( d.Participating() )
    {
        const elem::Grid& g = this->Grid();
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.RowShift();

        Int iStart,jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalWidth();

        const R* dBuffer = d.LockedBuffer();
        const Int dLDim = d.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            this->SetLocalRealPart( iLocal, jLocal, dBuffer[k*dLDim] );
        }
    }
}

template<typename T,typename Int>
void
DistMatrix<T,MC,MR,Int>::SetImagPartOfDiagonal
( const DistMatrix<BASE(T),STAR,MD,Int>& d, Int offset )
{
#ifndef RELEASE
    CallStackEntry entry("[MC,MR]::SetImagPartOfDiagonal");
    this->AssertSameGrid( d.Grid() );
    if( d.Height() != 1 )
        throw std::logic_error("d must be a row vector");
    const Int length = this->DiagonalLength( offset );
    if( length != d.Width() )
    {
        std::ostringstream msg;
        msg << "d is not of the same length as the diagonal:\n"
            << "  A ~ " << this->Height() << " x " << this->Width() << "\n"
            << "  d ~ " << d.Height() << " x " << d.Width() << "\n"
            << "  A diag length: " << length << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    typedef BASE(T) R;
    if( !IsComplex<T>::val )
        throw std::logic_error("Called complex-only routine with real data");

    if( d.Participating() )
    {
        const elem::Grid& g = this->Grid();
        const Int r = g.Height();
        const Int c = g.Width();
        const Int lcm = g.LCM();
        const Int colShift = this->ColShift();
        const Int rowShift = this->RowShift();
        const Int diagShift = d.RowShift();

        Int iStart,jStart;
        if( offset >= 0 )
        {
            iStart = diagShift;
            jStart = diagShift+offset;
        }
        else
        {
            iStart = diagShift-offset;
            jStart = diagShift;
        }

        const Int iLocalStart = (iStart-colShift) / r;
        const Int jLocalStart = (jStart-rowShift) / c;

        const Int localDiagLength = d.LocalWidth();
        const R* dBuffer = d.LockedBuffer();
        const Int dLDim = d.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<localDiagLength; ++k )
        {
            const Int iLocal = iLocalStart + k*(lcm/r);
            const Int jLocal = jLocalStart + k*(lcm/c);
            this->SetLocalImagPart( iLocal, jLocal, dBuffer[k*dLDim] );
        }
    }
}

template class DistMatrix<int,MC,MR,int>;
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,MC,  STAR,int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,MD,  STAR,int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,MR,  MC,  int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,MR,  STAR,int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,STAR,MC,  int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,STAR,MD,  int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,STAR,MR,  int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,STAR,STAR,int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,STAR,VC,  int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,STAR,VR,  int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,VC,  STAR,int>& A );
template DistMatrix<int,MC,MR,int>::DistMatrix( const DistMatrix<int,VR,  STAR,int>& A );

#ifndef DISABLE_FLOAT
template class DistMatrix<float,MC,MR,int>;
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,MC,  STAR,int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,MD,  STAR,int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,MR,  MC,  int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,MR,  STAR,int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,STAR,MC,  int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,STAR,MD,  int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,STAR,MR,  int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,STAR,STAR,int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,STAR,VC,  int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,STAR,VR,  int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,VC,  STAR,int>& A );
template DistMatrix<float,MC,MR,int>::DistMatrix( const DistMatrix<float,VR,  STAR,int>& A );
#endif // ifndef DISABLE_FLOAT

template class DistMatrix<double,MC,MR,int>;
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,MC,  STAR,int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,MD,  STAR,int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,MR,  MC,  int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,MR,  STAR,int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,STAR,MC,  int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,STAR,MD,  int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,STAR,MR,  int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,STAR,STAR,int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,STAR,VC,  int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,STAR,VR,  int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,VC,  STAR,int>& A );
template DistMatrix<double,MC,MR,int>::DistMatrix( const DistMatrix<double,VR,  STAR,int>& A );

#ifndef DISABLE_COMPLEX
#ifndef DISABLE_FLOAT
template class DistMatrix<Complex<float>,MC,MR,int>;
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,MC,  STAR,int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,MD,  STAR,int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,MR,  MC,  int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,MR,  STAR,int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,MC,  int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,MD,  int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,MR,  int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,STAR,int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,VC,  int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,VR,  int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,VC,  STAR,int>& A );
template DistMatrix<Complex<float>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<float>,VR,  STAR,int>& A );
#endif // ifndef DISABLE_FLOAT
template class DistMatrix<Complex<double>,MC,MR,int>;
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,MC,  STAR,int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,MD,  STAR,int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,MR,  MC,  int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,MR,  STAR,int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,MC,  int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,MD,  int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,MR,  int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,STAR,int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,VC,  int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,VR,  int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,VC,  STAR,int>& A );
template DistMatrix<Complex<double>,MC,MR,int>::DistMatrix( const DistMatrix<Complex<double>,VR,  STAR,int>& A );
#endif // ifndef DISABLE_COMPLEX

} // namespace elem
