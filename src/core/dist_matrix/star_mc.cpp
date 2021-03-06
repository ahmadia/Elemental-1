/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "elemental-lite.hpp"

namespace elem {

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::DistMatrix( const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (0,0,false,false,0,0,
   0,(g.InGrid() ? g.Row() : 0 ),
   0,0,g)
{ }

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::DistMatrix
( Int height, Int width, const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,false,false,0,0,
   0,(g.InGrid() ? g.Row() : 0),
   height,(g.InGrid() ? Length(width,g.Row(),0,g.Height()) : 0),
   g)
{ }

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::DistMatrix
( Int height, Int width, Int rowAlignment, const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,false,true,0,rowAlignment,
   0,(g.InGrid() ? Shift(g.Row(),rowAlignment,g.Height()) : 0),
   height,
   (g.InGrid() ? Length(width,g.Row(),rowAlignment,g.Height()) : 0),
   g)
{ }

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::DistMatrix
( Int height, Int width, Int rowAlignment, Int ldim, const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,false,true,0,rowAlignment,
   0,(g.InGrid() ? Shift(g.Row(),rowAlignment,g.Height()) : 0),
   height,
   (g.InGrid() ? Length(width,g.Row(),rowAlignment,g.Height()) : 0),
   ldim,g)
{ }

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::DistMatrix
( Int height, Int width, Int rowAlignment, const T* buffer, Int ldim, 
  const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,0,rowAlignment,
   0,(g.InGrid() ? Shift(g.Row(),rowAlignment,g.Height()) : 0),
   height,
   (g.InGrid() ? Length(width,g.Row(),rowAlignment,g.Height()) : 0),
   buffer,ldim,g)
{ }

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::DistMatrix
( Int height, Int width, Int rowAlignment, T* buffer, Int ldim, 
  const elem::Grid& g )
: AbstractDistMatrix<T,Int>
  (height,width,0,rowAlignment,
   0,(g.InGrid() ? Shift(g.Row(),rowAlignment,g.Height()) : 0),
   height,
   (g.InGrid() ? Length(width,g.Row(),rowAlignment,g.Height()) : 0),
   buffer,ldim,g)
{ }

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::DistMatrix( const DistMatrix<T,STAR,MC,Int>& A )
: AbstractDistMatrix<T,Int>(0,0,false,false,0,0,
  0,(A.Participating() ? A.RowRank() : 0),
  0,0,A.Grid())
{
#ifndef RELEASE
    CallStackEntry entry("DistMatrix[* ,MC]::DistMatrix");
#endif
    if( &A != this )
        *this = A;
    else
        throw std::logic_error("Tried to construct [* ,MC] with itself");
}

template<typename T,typename Int>
template<Distribution U,Distribution V>
DistMatrix<T,STAR,MC,Int>::DistMatrix( const DistMatrix<T,U,V,Int>& A )
: AbstractDistMatrix<T,Int>(0,0,false,false,0,0,
  0,(A.Participating() ? A.RowRank() : 0),
  0,0,A.Grid())
{
#ifndef RELEASE
    CallStackEntry entry("DistMatrix[* ,MC]::DistMatrix");
#endif
    if( STAR != U || MC != V || 
        reinterpret_cast<const DistMatrix<T,STAR,MC,Int>*>(&A) != this ) 
        *this = A;
    else
        throw std::logic_error("Tried to construct [* ,MC] with itself");
}

template<typename T,typename Int>
DistMatrix<T,STAR,MC,Int>::~DistMatrix()
{ }

template<typename T,typename Int>
elem::DistData<Int>
DistMatrix<T,STAR,MC,Int>::DistData() const
{
    elem::DistData<Int> data;
    data.colDist = STAR;
    data.rowDist = MC;
    data.colAlignment = 0;
    data.rowAlignment = this->rowAlignment_;
    data.diagPath = 0;
    data.grid = this->grid_;
    return data;
}

template<typename T,typename Int>
Int
DistMatrix<T,STAR,MC,Int>::ColStride() const
{ return 1; }

template<typename T,typename Int>
Int
DistMatrix<T,STAR,MC,Int>::RowStride() const
{ return this->grid_->Height(); }

template<typename T,typename Int>
Int
DistMatrix<T,STAR,MC,Int>::ColRank() const
{ return 0; }

template<typename T,typename Int>
Int
DistMatrix<T,STAR,MC,Int>::RowRank() const
{ return this->grid_->Row(); }

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::AlignWith( const elem::DistData<Int>& data )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::AlignWith");
    this->AssertFreeRowAlignment();
#endif
    const Grid& grid = *data.grid;
    this->SetGrid( grid );

    if( data.colDist == MC )
        this->rowAlignment_ = data.colAlignment;
    else if( data.rowDist == MC )
        this->rowAlignment_ = data.rowAlignment;
    else if( data.colDist == VC )
        this->rowAlignment_ = data.colAlignment % this->RowStride();
    else if( data.rowDist == VC )
        this->rowAlignment_ = data.rowAlignment % this->RowStride();
#ifndef RELEASE
    else throw std::logic_error("Nonsensical alignment");
#endif
    this->constrainedRowAlignment_ = true;
    this->SetShifts();
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::AlignWith( const AbstractDistMatrix<T,Int>& A )
{ this->AlignWith( A.DistData() ); }

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::AlignRowsWith( const elem::DistData<Int>& data )
{ this->AlignWith( data ); }

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::AlignRowsWith( const AbstractDistMatrix<T,Int>& A )
{ this->AlignWith( A.DistData() ); }

template<typename T,typename Int>
bool
DistMatrix<T,STAR,MC,Int>::AlignedWithDiagonal
( const elem::DistData<Int>& data, Int offset ) const
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::AlignedWithDiagonal");
#endif
    const Grid& grid = this->Grid();
    if( grid != *data.grid )
        return false;

    bool aligned;
    if( (data.colDist == MC   && data.rowDist == STAR) ||
        (data.colDist == STAR && data.rowDist == MC  ) )
    {
        const Int alignment = ( data.colDist==MC ? data.colAlignment
                                                 : data.rowAlignment );
        if( offset >= 0 )
        {
            const Int row = alignment;
            aligned = ( this->RowAlignment() == row );
        }
        else
        {
            const Int row = (alignment-offset) % this->RowStride();
            aligned = ( this->RowAlignment() == row );
        }
    }
    else aligned = false;
    return aligned;
}

template<typename T,typename Int>
bool
DistMatrix<T,STAR,MC,Int>::AlignedWithDiagonal
( const AbstractDistMatrix<T,Int>& A, Int offset ) const
{ return this->AlignedWithDiagonal( A.DistData(), offset ); }

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::AlignWithDiagonal
( const elem::DistData<Int>& data, Int offset )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::AlignWithDiagonal");
    this->AssertFreeRowAlignment();
#endif
    const Grid& grid = *data.grid;
    this->SetGrid( grid );

    if( (data.colDist == MC   && data.rowDist == STAR) ||
        (data.colDist == STAR && data.rowDist == MC  ) )
    {
        const Int alignment = ( data.colDist==MC ? data.colAlignment
                                                 : data.rowAlignment );
        if( offset >= 0 )
        {
            const Int row = alignment;
            this->rowAlignment_ = row;
        }
        else
        {
            const Int row = (alignment-offset) % this->RowStride();
            this->rowAlignment_ = row;
        }
        this->constrainedRowAlignment_ = true;
        this->SetShifts();
    }
#ifndef RELEASE
    else throw std::logic_error("Invalid diagonal alignment");
#endif
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::AlignWithDiagonal
( const AbstractDistMatrix<T,Int>& A, Int offset )
{ this->AlignWithDiagonal( A.DistData(), offset ); }

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::PrintBase
( std::ostream& os, const std::string msg ) const
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::PrintBase");
#endif
    const elem::Grid& g = this->Grid();
    if( g.Rank() == 0 && msg != "" )
        os << msg << std::endl;

    const Int height     = this->Height();
    const Int width      = this->Width();
    const Int localWidth = this->LocalWidth();
    const Int r          = g.Height();
    const Int rowShift   = this->RowShift();

    if( height == 0 || width == 0 || !g.InGrid() )
        return;

    // Only one process col needs to participate
    if( g.Col() == 0 )
    {
        std::vector<T> sendBuf(height*width,0);
        const T* thisBuffer = this->LockedBuffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            T* destCol = &sendBuf[(rowShift+jLocal*r)*height];
            const T* sourceCol = &thisBuffer[jLocal*thisLDim];
            MemCopy( destCol, sourceCol, height );
        }

        // If we are the root, allocate a receive buffer
        std::vector<T> recvBuf;
        if( g.Row() == 0 )
            recvBuf.resize( height*width );

        // Sum the contributions and send to the root
        mpi::Reduce
        ( &sendBuf[0], &recvBuf[0], height*width, mpi::SUM, 0, g.ColComm() );

        if( g.Row() == 0 )
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
    }
    mpi::Barrier( g.VCComm() );
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::Attach
( Int height, Int width, Int rowAlignment,
  T* buffer, Int ldim, const elem::Grid& g )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::Attach");
#endif
    this->Empty();

    this->grid_ = &g;
    this->height_ = height;
    this->width_ = width;
    this->rowAlignment_ = rowAlignment;
    this->viewing_ = true;
    this->SetRowShift();
    if( g.InGrid() )
    {
        const Int localWidth = Length(width,this->rowShift_,g.Height());
        this->matrix_.Attach( height, localWidth, buffer, ldim );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::LockedAttach
( Int height, Int width, Int rowAlignment,
  const T* buffer, Int ldim, const elem::Grid& g )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::LockedAttach");
#endif
    this->Empty();

    this->grid_ = &g;
    this->height_ = height;
    this->width_ = width;
    this->rowAlignment_ = rowAlignment;
    this->viewing_ = true;
    this->locked_ = true;
    this->SetRowShift();
    if( g.InGrid() )
    {
        const Int localWidth = Length(width,this->rowShift_,g.Height());
        this->matrix_.LockedAttach( height, localWidth, buffer, ldim );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::ResizeTo( Int height, Int width )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::ResizeTo");
    this->AssertNotLocked();
    if( height < 0 || width < 0 )
        throw std::logic_error("Height and width must be non-negative");
#endif
    this->height_ = height;
    this->width_ = width;
    if( this->Participating() )
        this->matrix_.ResizeTo
        ( height, Length(width,this->RowShift(),this->Grid().Height()) );
}

template<typename T,typename Int>
T
DistMatrix<T,STAR,MC,Int>::Get( Int i, Int j ) const
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MR]::Get");
    this->AssertValidEntry( i, j );
    if( !this->Participating() )
        throw std::logic_error("Should only be called by processes in grid");
#endif
    // We will determine the owner row of entry (i,j) and broadcast from that
    // row within each process column
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    T u;
    if( g.Row() == ownerRow )
    {
        const Int jLoc = (j-this->RowShift()) / g.Height();
        u = this->GetLocal(i,jLoc);
    }
    mpi::Broadcast( &u, 1, ownerRow, g.ColComm() );
    return u;
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::Set( Int i, Int j, T u )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::Set");
    this->AssertValidEntry( i, j );
#endif
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    if( g.Row() == ownerRow )
    {
        const Int jLoc = (j-this->RowShift()) / g.Height();
        this->SetLocal(i,jLoc,u);
    }
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::Update( Int i, Int j, T u )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::Update");
    this->AssertValidEntry( i, j );
#endif
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    if( g.Row() == ownerRow )
    {
        const Int jLoc = (j-this->RowShift()) / g.Height();
        this->UpdateLocal(i,jLoc,u);
    }
}

//
// Utility functions, e.g., SumOverRow
//

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::SumOverRow()
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::SumOverRow");
    this->AssertNotLocked();
#endif
    if( !this->Participating() )
        return;

    const Int localHeight = this->LocalHeight();
    const Int localWidth = this->LocalWidth();
    const Int localSize = std::max( localHeight*localWidth, mpi::MIN_COLL_MSG );

    this->auxMemory_.Require( 2*localSize );
    T* buffer = this->auxMemory_.Buffer();
    T* sendBuf = &buffer[0];
    T* recvBuf = &buffer[localSize];

    // Pack
    T* thisBuffer = this->Buffer();
    const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const T* thisCol = &thisBuffer[jLocal*thisLDim];
        T* sendBufCol = &sendBuf[jLocal*localHeight];
        MemCopy( sendBufCol, thisCol, localHeight );
    }

    // AllReduce sum
    mpi::AllReduce
    ( sendBuf, recvBuf, localSize, mpi::SUM, this->Grid().RowComm() );

    // Unpack
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const T* recvBufCol = &recvBuf[jLocal*localHeight];
        T* thisCol = &thisBuffer[jLocal*thisLDim];
        MemCopy( thisCol, recvBufCol, localHeight );
    }
    this->auxMemory_.Release();
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::AdjointFrom( const DistMatrix<T,VC,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::AdjointFrom");
#endif
    this->TransposeFrom( A, true );
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::TransposeFrom
( const DistMatrix<T,VC,STAR,Int>& A, bool conjugate )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::TransposeFrom");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Width(), A.Height() );
#endif
    const elem::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->rowAlignment_ = A.ColAlignment() % g.Height();
            this->SetRowShift();
        }
        this->ResizeTo( A.Width(), A.Height() );
    }
    if( !g.InGrid() )
        return;

    if( this->RowAlignment() == A.ColAlignment() % g.Height() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = g.Size();
        const Int row = g.Row();

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localHeightOfA = A.LocalHeight();
        const Int maxLocalHeightOfA = MaxLength(width,p);

        const Int portionSize = 
            std::max(height*maxLocalHeightOfA,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( (c+1)*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
        if( conjugate )
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            {
                T* destCol = &originalData[jLocal*height];
                const T* sourceCol = &ABuffer[jLocal];
                for( Int i=0; i<height; ++i )
                    destCol[i] = Conj( sourceCol[i*ALDim] );
            }
        }
        else
        {
 #ifdef HAVE_OPENMP
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            {
                T* destCol = &originalData[jLocal*height];
                const T* sourceCol = &ABuffer[jLocal];
                for( Int i=0; i<height; ++i )
                    destCol[i] = sourceCol[i*ALDim];
            }
        }

        // Communicate
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.RowComm() );

        // Unpack
        const Int rowShift = this->RowShift();
        const Int colAlignmentOfA = A.ColAlignment();
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<c; ++k )
        {
            const T* data = &gatheredData[k*portionSize];

            const Int colShiftOfA = Shift_( row+k*r, colAlignmentOfA, p );
            const Int rowOffset = (colShiftOfA-rowShift) / r;
            const Int localWidth = Length_( width, colShiftOfA, p );

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*height];
                T* thisCol = &thisBuffer[(rowOffset+jLocal*c)*thisLDim];
                MemCopy( thisCol, dataCol, height );
            }
        }
        this->auxMemory_.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [* ,MC]::AdjointFrom." << std::endl;
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = g.Size();
        const Int row = g.Row();
        const Int rank = g.VCRank();

        // Perform the SendRecv to make A have the same rowAlignment
        const Int rowAlignment = this->RowAlignment();
        const Int colAlignmentOfA = A.ColAlignment();
        const Int rowShift = this->RowShift();

        const Int sendRank = (rank+p+rowAlignment-colAlignmentOfA) % p;
        const Int recvRank = (rank+p+colAlignmentOfA-rowAlignment) % p;

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localHeightOfA = A.LocalHeight();
        const Int maxLocalHeightOfA = MaxLength(width,p);

        const Int portionSize = 
            std::max(height*maxLocalHeightOfA,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( (c+1)*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
        if( conjugate )
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            {
                T* destCol = &secondBuffer[jLocal*height];
                const T* sourceCol = &ABuffer[jLocal];
                for( Int i=0; i<height; ++i )
                    destCol[i] = Conj( sourceCol[i*ALDim] );
            }
        }
        else
        {
#ifdef HAVE_OPENMP
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localHeightOfA; ++jLocal )
            {
                T* destCol = &secondBuffer[jLocal*height];
                const T* sourceCol = &ABuffer[jLocal];
                for( Int i=0; i<height; ++i )
                    destCol[i] = sourceCol[i*ALDim];
            }
        }

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendRank, 0,
          firstBuffer,  portionSize, recvRank, 0, g.VCComm() );

        // Use the SendRecv as input to the AllGather
        mpi::AllGather
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

            const Int colShiftOfA = Shift_(row+r*k,rowAlignment,p);
            const Int rowOffset = (colShiftOfA-rowShift) / r;
            const Int localWidth = Length_( width, colShiftOfA, p );
            
#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*height];
                T* thisCol = &thisBuffer[(rowOffset+jLocal*c)*thisLDim];
                MemCopy( thisCol, dataCol, height );
            }
        }
        this->auxMemory_.Release();
    }
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,MC,MR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [MC,MR]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    std::auto_ptr<DistMatrix<T,STAR,VR,Int> > A_STAR_VR
    ( new DistMatrix<T,STAR,VR,Int>(g) );
    *A_STAR_VR = A;

    std::auto_ptr<DistMatrix<T,STAR,VC,Int> > A_STAR_VC
    ( new DistMatrix<T,STAR,VC,Int>(true,this->RowAlignment(),g) );
    *A_STAR_VC = *A_STAR_VR;
    delete A_STAR_VR.release(); // lowers memory highwater

    *this = *A_STAR_VC;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,MC,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [MC,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    std::auto_ptr<DistMatrix<T,MC,MR,Int> > A_MC_MR
    ( new DistMatrix<T,MC,MR,Int>(g) );
    *A_MC_MR   = A;

    std::auto_ptr<DistMatrix<T,STAR,VR,Int> > A_STAR_VR
    ( new DistMatrix<T,STAR,VR,Int>(g) );
    *A_STAR_VR = *A_MC_MR;
    delete A_MC_MR.release(); // lowers memory highwater

    std::auto_ptr<DistMatrix<T,STAR,VC,Int> > A_STAR_VC
    ( new DistMatrix<T,STAR,VC,Int>(true,this->RowAlignment(),g) );
    *A_STAR_VC = *A_STAR_VR;
    delete A_STAR_VR.release(); // lowers memory highwater

    *this = *A_STAR_VC;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,STAR,MR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [* ,MR]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    if( A.Height() == 1 )
    {
        if( !this->Viewing() )
            this->ResizeTo( 1, A.Width() );
        if( !g.InGrid() )
            return *this;

        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = g.Size();
        const Int myRow = g.Row();
        const Int rankCM = g.VCRank();
        const Int rankRM = g.VRRank();
        const Int rowAlignment = this->RowAlignment();
        const Int rowShift = this->RowShift();
        const Int rowAlignmentOfA = A.RowAlignment();
        const Int rowShiftOfA = A.RowShift();

        const Int width = this->Width();
        const Int maxLocalVectorWidth = MaxLength(width,p);
        const Int portionSize = std::max(maxLocalVectorWidth,mpi::MIN_COLL_MSG);

        const Int rowShiftVC = Shift(rankCM,rowAlignment,p);
        const Int rowShiftVROfA = Shift(rankRM,rowAlignmentOfA,p);
        const Int sendRankCM = (rankCM+(p+rowShiftVROfA-rowShiftVC)) % p;
        const Int recvRankRM = (rankRM+(p+rowShiftVC-rowShiftVROfA)) % p;
        const Int recvRankCM = (recvRankRM/c)+r*(recvRankRM%c);

        this->auxMemory_.Require( (c+1)*portionSize );
        T* buffer = this->auxMemory_.Buffer();
        T* sendBuf = &buffer[0];
        T* recvBuf = &buffer[c*portionSize];

        // A[* ,VR] <- A[* ,MR]
        {
            const Int shift = Shift(rankRM,rowAlignmentOfA,p);
            const Int offset = (shift-rowShiftOfA) / c;
            const Int thisLocalWidth = Length(width,shift,p);

            const T* ABuffer = A.LockedBuffer();
            const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                sendBuf[jLocal] = ABuffer[(offset+jLocal*r)*ALDim];
        }

        // A[* ,VC] <- A[* ,VR]
        mpi::SendRecv
        ( sendBuf, portionSize, sendRankCM, 0,
          recvBuf, portionSize, recvRankCM, mpi::ANY_TAG, g.VCComm() );

        // A[* ,MC] <- A[* ,VC]
        mpi::AllGather
        ( recvBuf, portionSize,
          sendBuf, portionSize, g.RowComm() );

        // Unpack
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int k=0; k<c; ++k )
        {
            const T* data = &sendBuf[k*portionSize];

            const Int shift = Shift_(myRow+r*k,rowAlignment,p);
            const Int offset = (shift-rowShift) / r;
            const Int thisLocalWidth = Length_(width,shift,p);

            for( Int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
                thisBuffer[(offset+jLocal*c)*thisLDim] = data[jLocal];
        }
        this->auxMemory_.Release();
    }
    else
    {
        std::auto_ptr<DistMatrix<T,STAR,VR,Int> > A_STAR_VR
        ( new DistMatrix<T,STAR,VR,Int>(g) );
        *A_STAR_VR = A;

        std::auto_ptr<DistMatrix<T,STAR,VC,Int> > A_STAR_VC
        ( new DistMatrix<T,STAR,VC,Int>(true,this->RowAlignment(),g) );
        *A_STAR_VC = *A_STAR_VR;
        delete A_STAR_VR.release(); // lowers memory highwater

        *this = *A_STAR_VC;
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,MD,STAR,Int>& A )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [MD,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    throw std::logic_error("[* ,MC] = [MD,* ] not yet implemented");
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,STAR,MD,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [* ,MD]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    throw std::logic_error("[* ,MC] = [MD,* ] not yet implemented");
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,MR,MC,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [MR,MC]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
#ifdef VECTOR_WARNINGS
    if( A.Height() == 1 && g.Rank() == 0 )
    {
        std::cerr << 
          "The vector version of [* ,MC] <- [MR,MC] is not yet written, but"
          " it would only require a modification of the vector version of "
          "[* ,MR] <- [MC,MR]." << std::endl;
    }
#endif
#ifdef CACHE_WARNINGS
    if( A.Height() != 1 && g.Rank() == 0 )
    {
        std::cerr << 
          "The redistribution [* ,MC] <- [MR,MC] potentially causes a large"
          " amount of cache-thrashing. If possible, avoid it. "
          "Unfortunately, the following routines are not yet implemented: \n"
          << "  [MC,* ].(Conjugate)TransposeFrom([MR,MC])" << std::endl;
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
        return *this;

    if( this->RowAlignment() == A.RowAlignment() )
    {
        const Int c = g.Width();
        const Int height = this->Height();
        const Int localWidth = this->LocalWidth();
        const Int localHeightOfA = A.LocalHeight();
        const Int maxLocalHeightOfA = MaxLength(height,c);

        const Int portionSize = 
            std::max(maxLocalHeightOfA*localWidth,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( (c+1)*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* ACol = &ABuffer[jLocal*ALDim];
            T* originalDataCol = &originalData[jLocal*localHeightOfA];
            MemCopy( originalDataCol, ACol, localHeightOfA );
        }

        // Communicate
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.RowComm() );

        // Unpack
        const Int colAlignmentOfA = A.ColAlignment();
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<c; ++k )
        {
            const T* data = &gatheredData[k*portionSize];

            const Int colShift = Shift_( k, colAlignmentOfA, c );
            const Int localHeight = Length_( height, colShift, c );

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &thisBuffer[colShift+jLocal*thisLDim];
                const T* sourceCol = &data[jLocal*localHeight];
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    destCol[iLocal*c] = sourceCol[iLocal];
            }
        }
        this->auxMemory_.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [* ,MC] <- [MR,MC]." << std::endl;
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int row = g.Row();

        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentOfA = A.RowAlignment();
        const Int sendRow = (row+r+rowAlignment-rowAlignmentOfA) % r;
        const Int recvRow = (row+r+rowAlignmentOfA-rowAlignment) % r;

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localWidth = this->LocalWidth();
        const Int localHeightOfA = A.LocalHeight();
        const Int localWidthOfA = A.LocalWidth();
        const Int maxLocalHeightOfA = MaxLength(height,c);
        const Int maxLocalWidth = MaxLength(width,r);

        const Int portionSize = 
            std::max(maxLocalHeightOfA*maxLocalWidth,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( (c+1)*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidthOfA; ++jLocal )
        {
            const T* ACol = &ABuffer[jLocal*ALDim];
            T* secondBufferCol = &secondBuffer[jLocal*localHeightOfA];
            MemCopy( secondBufferCol, ACol, localHeightOfA );
        }

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendRow, 0,
          firstBuffer,  portionSize, recvRow, mpi::ANY_TAG, g.ColComm() );

        // Use the output of the SendRecv as input to the AllGather
        mpi::AllGather
        ( firstBuffer,  portionSize,
          secondBuffer, portionSize, g.RowComm() );

        // Unpack the contents of each member of the process row
        const Int colAlignmentOfA = A.ColAlignment();
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<c; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const Int colShift = Shift_( k, colAlignmentOfA, c );
            const Int localHeight = Length_( height, colShift, c );
#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for 
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                T* destCol = &thisBuffer[colShift+jLocal*thisLDim];
                const T* sourceCol = &data[jLocal*localHeight];
                for( Int iLocal=0; iLocal<localHeight; ++iLocal )
                    destCol[iLocal*c] = sourceCol[iLocal];
            }
        }
        this->auxMemory_.Release();
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,MR,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [MR,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    DistMatrix<T,MR,MC,Int> A_MR_MC(g);

    A_MR_MC = A;
    *this = A_MR_MC;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,STAR,MC,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [* ,MC]");
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
            if( g.InGrid() )
                this->rowShift_ = A.RowShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return *this;

    if( this->RowAlignment() == A.RowAlignment() )
    {
        this->matrix_ = A.LockedMatrix();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [* ,MC] <- [* ,MC]." << std::endl;
#endif
        const Int rank = g.Row();
        const Int r = g.Height();

        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentOfA = A.RowAlignment();

        const Int sendRank = (rank+r+rowAlignment-rowAlignmentOfA) % r;
        const Int recvRank = (rank+r+rowAlignmentOfA-rowAlignment) % r;

        const Int height = this->Height();
        const Int localWidth = this->LocalWidth();
        const Int localWidthOfA = A.LocalWidth();

        const Int sendSize = height * localWidthOfA;
        const Int recvSize = height * localWidth;

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
        for( Int jLocal=0; jLocal<localWidthOfA; ++jLocal )
        {
            const T* ACol = &ABuffer[jLocal*ALDim];
            T* sendBufferCol = &sendBuffer[jLocal*height];
            MemCopy( sendBufferCol, ACol, height );
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
            const T* recvBufferCol = &recvBuffer[jLocal*height];
            T* thisCol = &thisBuffer[jLocal*thisLDim];
            MemCopy( thisCol, recvBufferCol, height );
        }
        this->auxMemory_.Release();
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,VC,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [VC,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    std::auto_ptr<DistMatrix<T,VR,STAR,Int> > A_VR_STAR
    ( new DistMatrix<T,VR,STAR,Int>(g) );
    *A_VR_STAR = A;

    std::auto_ptr<DistMatrix<T,MR,MC,Int> > A_MR_MC
    ( new DistMatrix<T,MR,MC,Int>(false,true,0,this->RowAlignment(),g) );
    *A_MR_MC = *A_VR_STAR;
    delete A_VR_STAR.release();

    *this = *A_MR_MC;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,STAR,VC,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [* ,VC]");
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
            this->rowAlignment_ = A.RowAlignment() % g.Height();
            this->SetRowShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }
    if( !g.InGrid() )
        return *this;

    if( this->RowAlignment() == A.RowAlignment() % g.Height() )
    {
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = g.Size();
        const Int row = g.Row();

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localWidthOfA = A.LocalWidth();
        const Int maxLocalWidthOfA = MaxLength(width,p);

        const Int portionSize = 
            std::max(height*maxLocalWidthOfA,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( (c+1)*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* originalData = &buffer[0];
        T* gatheredData = &buffer[portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidthOfA; ++jLocal )
        {
            const T* ACol = &ABuffer[jLocal*ALDim];
            T* originalDataCol = &originalData[jLocal*height];
            MemCopy( originalDataCol, ACol, height );
        }

        // Communicate
        mpi::AllGather
        ( originalData, portionSize,
          gatheredData, portionSize, g.RowComm() );

        // Unpack
        const Int rowShift = this->RowShift();
        const Int rowAlignmentOfA = A.RowAlignment();
        T* thisBuffer = this->Buffer();
        const Int thisLDim = this->LDim();
#if defined(HAVE_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( Int k=0; k<c; ++k )
        {
            const T* data = &gatheredData[k*portionSize];

            const Int rowShiftOfA = Shift_( row+k*r, rowAlignmentOfA, p );
            const Int rowOffset = (rowShiftOfA-rowShift) / r;
            const Int localWidth = Length_( width, rowShiftOfA, p );

#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*height];
                T* thisCol = &thisBuffer[(rowOffset+jLocal*c)*thisLDim];
                MemCopy( thisCol, dataCol, height );
            }
        }
        this->auxMemory_.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.Rank() == 0 )
            std::cerr << "Unaligned [* ,MC] <- [* ,VC]." << std::endl;
#endif
        const Int r = g.Height();
        const Int c = g.Width();
        const Int p = g.Size();
        const Int row = g.Row();
        const Int rank = g.VCRank();

        // Perform the SendRecv to make A have the same rowAlignment
        const Int rowAlignment = this->RowAlignment();
        const Int rowAlignmentOfA = A.RowAlignment();
        const Int rowShift = this->RowShift();

        const Int sendRank = (rank+p+rowAlignment-rowAlignmentOfA) % p;
        const Int recvRank = (rank+p+rowAlignmentOfA-rowAlignment) % p;

        const Int height = this->Height();
        const Int width = this->Width();
        const Int localWidthOfA = A.LocalWidth();
        const Int maxLocalWidthOfA = MaxLength(width,p);

        const Int portionSize = 
            std::max(height*maxLocalWidthOfA,mpi::MIN_COLL_MSG);

        this->auxMemory_.Require( (c+1)*portionSize );

        T* buffer = this->auxMemory_.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[portionSize];

        // Pack
        const T* ABuffer = A.LockedBuffer();
        const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
        #pragma omp parallel for
#endif
        for( Int jLocal=0; jLocal<localWidthOfA; ++jLocal )
        {
            const T* ACol = &ABuffer[jLocal*ALDim];
            T* secondBufferCol = &secondBuffer[jLocal*height];
            MemCopy( secondBufferCol, ACol, height );
        }

        // Perform the SendRecv: puts the new data into the first buffer
        mpi::SendRecv
        ( secondBuffer, portionSize, sendRank, 0,
          firstBuffer,  portionSize, recvRank, 0, g.VCComm() );

        // Use the SendRecv as input to the AllGather
        mpi::AllGather
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

            const Int rowShiftOfA = Shift_(row+r*k,rowAlignment,p);
            const Int rowOffset = (rowShiftOfA-rowShift) / r;
            const Int localWidth = Length_( width, rowShiftOfA, p );
            
#if defined(HAVE_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( Int jLocal=0; jLocal<localWidth; ++jLocal )
            {
                const T* dataCol = &data[jLocal*height];
                T* thisCol = &thisBuffer[(rowOffset+jLocal*c)*thisLDim];
                MemCopy( thisCol, dataCol, height );
            }
        }
        this->auxMemory_.Release();
    }
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,VR,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [VR,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    DistMatrix<T,MR,MC,Int> A_MR_MC(g);

    A_MR_MC = A;
    *this = A_MR_MC;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,STAR,VR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [* ,VR]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    const elem::Grid& g = this->Grid();
    DistMatrix<T,STAR,VC,Int> A_STAR_VC(true,this->RowAlignment(),g);
    *this = A_STAR_VC = A;
    return *this;
}

template<typename T,typename Int>
const DistMatrix<T,STAR,MC,Int>&
DistMatrix<T,STAR,MC,Int>::operator=( const DistMatrix<T,STAR,STAR,Int>& A )
{ 
#ifndef RELEASE
    CallStackEntry entry("[* ,MC] = [* ,* ]");
    this->AssertNotLocked();
    this->AssertSameGrid( A.Grid() );
    if( this->Viewing() )
        this->AssertSameSize( A.Height(), A.Width() );
#endif
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );
    if( !this->Participating() )
        return *this;

    const Int r = this->Grid().Height();
    const Int rowShift = this->RowShift();

    const Int localHeight = this->LocalHeight();
    const Int localWidth = this->LocalWidth();

    T* thisBuffer = this->Buffer();
    const Int thisLDim = this->LDim();
    const T* ABuffer = A.LockedBuffer();
    const Int ALDim = A.LDim();
#ifdef HAVE_OPENMP
    #pragma omp parallel for
#endif
    for( Int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const T* ACol = &ABuffer[(rowShift+jLocal*r)*ALDim];
        T* thisCol = &thisBuffer[jLocal*thisLDim];
        MemCopy( thisCol, ACol, localHeight );
    }
    return *this;
}

//
// Routines which explicitly work in the complex plane
//

template<typename T,typename Int>
BASE(T)
DistMatrix<T,STAR,MC,Int>::GetRealPart( Int i, Int j ) const
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MR]::GetRealPart");
    this->AssertValidEntry( i, j );
    if( !this->Participating() )
        throw std::logic_error("Should only be called by processes in grid");
#endif
    typedef BASE(T) R;

    // We will determine the owner row of entry (i,j) and broadcast from that
    // row within each process column
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    R u;
    if( g.Row() == ownerRow )
    {
        const Int jLocal = (j-this->RowShift()) / g.Height();
        u = this->GetLocalRealPart( i, jLocal );
    }
    mpi::Broadcast( &u, 1, ownerRow, g.ColComm() );
    return u;
}

template<typename T,typename Int>
BASE(T)
DistMatrix<T,STAR,MC,Int>::GetImagPart( Int i, Int j ) const
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MR]::GetImagPart");
    this->AssertValidEntry( i, j );
    if( !this->Participating() )
        throw std::logic_error("Should only be called by processes in grid");
#endif
    typedef BASE(T) R;

    // We will determine the owner row of entry (i,j) and broadcast from that
    // row within each process column
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    R u;
    if( g.Row() == ownerRow )
    {
        const Int jLocal = (j-this->RowShift()) / g.Height();
        u = this->GetLocalImagPart( i, jLocal );
    }
    mpi::Broadcast( &u, 1, ownerRow, g.ColComm() );
    return u;
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::SetRealPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::SetRealPart");
    this->AssertValidEntry( i, j );
#endif
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    if( g.Row() == ownerRow )
    {
        const Int jLocal = (j-this->RowShift()) / g.Height();
        this->SetLocalRealPart( i, jLocal, u );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::SetImagPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::SetImagPart");
    this->AssertValidEntry( i, j );
#endif
    if( !IsComplex<T>::val )
        throw std::logic_error("Called complex-only routine with real data");

    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    if( g.Row() == ownerRow )
    {
        const Int jLocal = (j-this->RowShift()) / g.Height();
        this->SetLocalImagPart( i, jLocal, u );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::UpdateRealPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::UpdateRealPart");
    this->AssertValidEntry( i, j );
#endif
    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    if( g.Row() == ownerRow )
    {
        const Int jLocal = (j-this->RowShift()) / g.Height();
        this->UpdateLocalRealPart( i, jLocal, u );
    }
}

template<typename T,typename Int>
void
DistMatrix<T,STAR,MC,Int>::UpdateImagPart( Int i, Int j, BASE(T) u )
{
#ifndef RELEASE
    CallStackEntry entry("[* ,MC]::UpdateImagPart");
    this->AssertValidEntry( i, j );
#endif
    if( !IsComplex<T>::val )
        throw std::logic_error("Called complex-only routine with real data");

    const elem::Grid& g = this->Grid();
    const Int ownerRow = (j + this->RowAlignment()) % g.Height();

    if( g.Row() == ownerRow )
    {
        const Int jLocal = (j-this->RowShift()) / g.Height();
        this->UpdateLocalImagPart( i, jLocal, u );
    }
}

template class DistMatrix<int,STAR,MC,int>;
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,MC,  MR,  int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,MC,  STAR,int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,MD,  STAR,int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,MR,  MC,  int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,MR,  STAR,int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,STAR,MD,  int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,STAR,MR,  int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,STAR,STAR,int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,STAR,VC,  int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,STAR,VR,  int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,VC,  STAR,int>& A );
template DistMatrix<int,STAR,MC,int>::DistMatrix( const DistMatrix<int,VR,  STAR,int>& A );

#ifndef DISABLE_FLOAT
template class DistMatrix<float,STAR,MC,int>;
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,MC,  MR,  int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,MC,  STAR,int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,MD,  STAR,int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,MR,  MC,  int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,MR,  STAR,int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,STAR,MD,  int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,STAR,MR,  int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,STAR,STAR,int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,STAR,VC,  int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,STAR,VR,  int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,VC,  STAR,int>& A );
template DistMatrix<float,STAR,MC,int>::DistMatrix( const DistMatrix<float,VR,  STAR,int>& A );
#endif // ifndef DISABLE_FLOAT

template class DistMatrix<double,STAR,MC,int>;
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,MC,  MR,  int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,MC,  STAR,int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,MD,  STAR,int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,MR,  MC,  int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,MR,  STAR,int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,STAR,MD,  int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,STAR,MR,  int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,STAR,STAR,int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,STAR,VC,  int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,STAR,VR,  int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,VC,  STAR,int>& A );
template DistMatrix<double,STAR,MC,int>::DistMatrix( const DistMatrix<double,VR,  STAR,int>& A );

#ifndef DISABLE_COMPLEX
#ifndef DISABLE_FLOAT
template class DistMatrix<Complex<float>,STAR,MC,int>;
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,MC,  MR,  int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,MC,  STAR,int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,MD,  STAR,int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,MR,  MC,  int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,MR,  STAR,int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,MD,  int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,MR,  int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,STAR,int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,VC,  int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,STAR,VR,  int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,VC,  STAR,int>& A );
template DistMatrix<Complex<float>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<float>,VR,  STAR,int>& A );
#endif // ifndef DISABLE_FLOAT
template class DistMatrix<Complex<double>,STAR,MC,int>;
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,MC,  MR,  int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,MC,  STAR,int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,MD,  STAR,int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,MR,  MC,  int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,MR,  STAR,int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,MD,  int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,MR,  int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,STAR,int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,VC,  int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,STAR,VR,  int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,VC,  STAR,int>& A );
template DistMatrix<Complex<double>,STAR,MC,int>::DistMatrix( const DistMatrix<Complex<double>,VR,  STAR,int>& A );
#endif // ifndef DISABLE_COMPLEX

} // namespace elem
