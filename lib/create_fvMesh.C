/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2008 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Description
    Construct fvMesh from cell shapes

\*---------------------------------------------------------------------------*/

#include "create_fvMesh.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
namespace Foam
{
    //---------------------------------------------------------------------------
    labelListList ext_cellShapePointCells
    (
	const cellShapeList& c,
	const pointField& the_points
    )
    {
	typedef DynamicList< label, primitiveMesh::cellsPerPoint_ > TDynamicList;
	List< TDynamicList > pc(the_points.size());

	// For each cell
	forAll(c, i)
	{
	    // For each vertex
	    const labelList& labels = c[i];
	    
	    forAll(labels, j)
            {
		// Set working point label
		label curPoint = labels[j];
		TDynamicList& curPointCells = pc[curPoint];
		
		// Enter the cell label in the point's cell list
		curPointCells.append(i);
	    }
	}

	labelListList pointCellAddr(pc.size());
	
	forAll (pc, pointI)
        {
	    TDynamicList& curPointCells = pc[pointI];
#if ( __FOAM_VERSION__ >= 010500 )
	    pointCellAddr[pointI] = curPointCells;
#else
	    pointCellAddr[pointI] = curPointCells.shrink();
#endif
	}
	
	return pointCellAddr;
    } 


    //---------------------------------------------------------------------------
    labelList ext_facePatchFaceCells
    (
	const faceList& patchFaces,
	const labelListList& pointCells,
	const faceListList& cellsFaceShapes,
	const label patchID
    )
    {
	register bool found;

	labelList FaceCells(patchFaces.size());
	
	forAll(patchFaces, fI)
	{
	    found = false;
	    
	    const face& curFace = patchFaces[fI];
	    const labelList& facePoints = patchFaces[fI];
	    
	    forAll(facePoints, pointI)
	    {
		const labelList& facePointCells = pointCells[facePoints[pointI]];
		
		forAll(facePointCells, cellI)
		{
		    faceList cellFaces = cellsFaceShapes[facePointCells[cellI]];
		    
		    forAll(cellFaces, cellFace)
		    {
			if (cellFaces[cellFace] == curFace)
			{
			    // Found the cell corresponding to this face
			    FaceCells[fI] = facePointCells[cellI];
			    
			    found = true;
			}
			if (found) break;
		    }
		    if (found) break;
		}
		if (found) break;
	    }
	    
	    if (!found)
	    {
		FatalErrorIn
		(
		    "ext_facePatchFaceCells(const faceList& patchFaces,"
		    "const labelListList& pointCells,"
		    "const faceListList& cellsFaceShapes,"
		    "const label patchID)"
                )   << "face " << fI << " in patch " << patchID
		    << " does not have neighbour cell"
		    << " face: " << patchFaces[fI]
		    << abort(FatalError);
	    }
	}
	
	return FaceCells;
    }


    //---------------------------------------------------------------------------
    fvMeshPtr create_fvMesh
    (
	const IOobject& io,
	const pointField& points,
	const cellShapeList& cellsAsShapes,
	const faceListList& boundaryFaces,
	const wordList& boundaryPatchNames,
	const wordList& boundaryPatchTypes,
	const word& defaultBoundaryPatchName,
	const word& defaultBoundaryPatchType,
	const wordList& boundaryPatchPhysicalTypes,
	const bool syncPar
    )
    {
	// Calculate the faces of all cells
	// Initialise maximum possible numer of mesh faces to 0
	label maxFaces = 0;
	
	// Set up a list of face shapes for each cell
	faceListList cellsFaceShapes( cellsAsShapes.size() );
	cellList cells( cellsAsShapes.size() );
	
	forAll(cellsFaceShapes, cellI)
	{
	    cellsFaceShapes[cellI] = cellsAsShapes[cellI].faces();
	    
	    cells[cellI].setSize(cellsFaceShapes[cellI].size());
	    
	    // Initialise cells to -1 to flag undefined faces
	    static_cast<labelList&>(cells[cellI]) = -1;
	    
	    // Count maximum possible numer of mesh faces
	    maxFaces += cellsFaceShapes[cellI].size();
	}
	
	// Set size of faces array to maximum possible number of mesh faces
	faceList faces(maxFaces);
	
	// Initialise number of faces to 0
	label nFaces = 0;
	
	// set reference to point-cell addressing
	labelListList PointCells = ext_cellShapePointCells( cellsAsShapes, points );
	
	bool found = false;
	
	forAll(cells, cellI)
        {
	    // Note:
	    // Insertion cannot be done in one go as the faces need to be
	    // added into the list in the increasing order of neighbour
	    // cells.  Therefore, all neighbours will be detected first
	    // and then added in the correct order.
	    
	    const faceList& curFaces = cellsFaceShapes[cellI];
	    
	    // Record the neighbour cell
	    labelList neiCells(curFaces.size(), -1);
	    
	    // Record the face of neighbour cell
	    labelList faceOfNeiCell(curFaces.size(), -1);
	    
	    label nNeighbours = 0;
	    
	    // For all faces ...
	    forAll(curFaces, faceI)
	    {
		// Skip faces that have already been matched
		if (cells[cellI][faceI] >= 0) continue;
		
		found = false;
		
		const face& curFace = curFaces[faceI];
		
		// Get the list of labels
		const labelList& curPoints = curFace;
		
		// For all points
		forAll(curPoints, pointI)
		{
		    // dGget the list of cells sharing this point
		    const labelList& curNeighbours =
			PointCells[curPoints[pointI]];
		    
		    // For all neighbours
		    forAll(curNeighbours, neiI)
		    {
			label curNei = curNeighbours[neiI];
			
			// Reject neighbours with the lower label
			if (curNei > cellI)
			{
			    // Get the list of search faces
			    const faceList& searchFaces = cellsFaceShapes[curNei];
			    
			    forAll(searchFaces, neiFaceI)
			    {
				if (searchFaces[neiFaceI] == curFace)
				{
				    // Match!!
				    found = true;
				    
				    // Record the neighbour cell and face
				    neiCells[faceI] = curNei;
				    faceOfNeiCell[faceI] = neiFaceI;
				    nNeighbours++;
				    
				    break;
				}
			    }
			    if (found) break;
			}
			if (found) break;
		    }
		    if (found) break;
		} // End of current points
	    }  // End of current faces
	    
	    // Add the faces in the increasing order of neighbours
	    for (label neiSearch = 0; neiSearch < nNeighbours; neiSearch++)
	    {
		// Find the lowest neighbour which is still valid
		label nextNei = -1;
		label minNei = cells.size();
		
		forAll (neiCells, ncI)
		{
		    if (neiCells[ncI] > -1 && neiCells[ncI] < minNei)
		    {
			nextNei = ncI;
			    minNei = neiCells[ncI];
		    }
		}
		
		if (nextNei > -1)
		{
		    // Add the face to the list of faces
		    faces[nFaces] = curFaces[nextNei];
		    
		    // Set cell-face and cell-neighbour-face to current face label
		    cells[cellI][nextNei] = nFaces;
		    cells[neiCells[nextNei]][faceOfNeiCell[nextNei]] = nFaces;
		    
		    // Stop the neighbour from being used again
		    neiCells[nextNei] = -1;
		    
		    // Increment number of faces counter
		    nFaces++;
		}
		else
		{
		    FatalErrorIn
		    (
			"ext_fvMesh::create\n"
			"(\n"
			"    const IOobject&,\n"
			"    const Xfer<pointField>&,\n"
			"    const cellShapeList& cellsAsShapes,\n"
			"    const faceListList& boundaryFaces,\n"
			"    const wordList& boundaryPatchTypes,\n"
			"    const wordList& boundaryPatchNames,\n"
			"    const word& defaultBoundaryPatchType\n"
			")"
		    )   << "Error in internal face insertion"
			<< abort(FatalError);
		}
	    }
	}
	
	// Do boundary faces
	
	labelList patchSizes(boundaryFaces.size(), -1);
	labelList patchStarts(boundaryFaces.size(), -1);
	
	forAll (boundaryFaces, patchI)
	{
	    const faceList& patchFaces = boundaryFaces[patchI];
	    
	    labelList curPatchFaceCells =
		ext_facePatchFaceCells
		(
		    patchFaces,
		    PointCells,
		    cellsFaceShapes,
		    patchI
		);
	    
	    // Grab the start label
	    label curPatchStart = nFaces;
	    
	    forAll (patchFaces, faceI)
	    {
		const face& curFace = patchFaces[faceI];
		
		const label cellInside = curPatchFaceCells[faceI];
		
		faces[nFaces] = curFace;
		
		// get faces of the cell inside
		const faceList& facesOfCellInside = cellsFaceShapes[cellInside];
		
		bool found = false;
		
		forAll (facesOfCellInside, cellFaceI)
		{
		    if (facesOfCellInside[cellFaceI] == curFace)
		    {
			if (cells[cellInside][cellFaceI] >= 0)
			{
			    FatalErrorIn
			    (
				"ext_fvMesh::create\n"
				"(\n"
				"    const IOobject&,\n"
				"    const Xfer<pointField>&,\n"
				"    const cellShapeList& cellsAsShapes,\n"
				"    const faceListList& boundaryFaces,\n"
				"    const wordList& boundaryPatchTypes,\n"
				"    const wordList& boundaryPatchNames,\n"
				"    const word& defaultBoundaryPatchType\n"
				")"
			    )   << "Trying to specify a boundary face " << curFace
				<< " on the face on cell " << cellInside
				<< " which is either an internal face or already "
				<< "belongs to some other patch.  This is face "
				<< faceI << " of patch "
				<< patchI << " named "
				<< boundaryPatchNames[patchI] << "."
				<< abort(FatalError);
			}
			
			found = true;
			
			cells[cellInside][cellFaceI] = nFaces;
			
			break;
		    }
		}
		
		if (!found)
		{
		    FatalErrorIn("ext_fvMesh::create(... construct from shapes...)")
			<< "face " << faceI << " of patch " << patchI
			<< " does not seem to belong to cell " << cellInside
			<< " which, according to the addressing, "
			<< "should be next to it."
			<< abort(FatalError);
		}
		
		// increment the counter of faces
		nFaces++;
	    }
	    
	    patchSizes[patchI] = nFaces - curPatchStart;
	    patchStarts[patchI] = curPatchStart;
	}
	
	// Grab "non-existing" faces and put them into a default patch
	
	label defaultPatchStart = nFaces;
	
	forAll(cells, cellI)
	{
	    labelList& curCellFaces = cells[cellI];
	    
	    forAll(curCellFaces, faceI)
	    {
		if (curCellFaces[faceI] == -1) // "non-existent" face
		{
		    curCellFaces[faceI] = nFaces;
		    faces[nFaces] = cellsFaceShapes[cellI][faceI];
		    
		    nFaces++;
		}
	    }
	}
	
	// Reset the size of the face list
	faces.setSize(nFaces);
	
	// Creation of the fvMesh
	fvMesh* mesh = new fvMesh
	( 
	    io,
#if ( __FOAM_VERSION__ >= 010600 )
	    Xfer< pointField >( points ),
	    Xfer< faceList >( faces ),
	    Xfer< cellList >( cells ),
#else
	    points,
	    faces,
	    cells,
#endif
	    syncPar
	);
	
	// Warning: Patches can only be added once the face list is
	// completed, as they hold a subList of the face list
	polyBoundaryMesh boundary
	( 
	    io, 
	    *mesh, 
	    boundaryFaces.size() + 1 // add room for a default patch
	);
	
	forAll (boundaryFaces, patchI)
	{
	    // add the patch to the list
	    boundary.set
	    (
		patchI,
		polyPatch::New
		(
		    boundaryPatchTypes[patchI],
		    boundaryPatchNames[patchI],
		    patchSizes[patchI],
		    patchStarts[patchI],
		    patchI,
		    boundary
		)
	    );
	    
	    if
	    (
		boundaryPatchPhysicalTypes.size()
	     && boundaryPatchPhysicalTypes[patchI].size()
	    )
	    {
		boundary[patchI].physicalType() =
		    boundaryPatchPhysicalTypes[patchI];
	    }
	}
	
	label nAllPatches = boundaryFaces.size();
	
	if (nFaces > defaultPatchStart)
	{
	    WarningIn("ext_fvMesh::create(... construct from shapes...)")
		<< "Found " << nFaces - defaultPatchStart
		<< " undefined faces in mesh; adding to default patch." << endl;
	    
	    boundary.set
	    (
		nAllPatches,
		polyPatch::New
		(
		    defaultBoundaryPatchType,
		    defaultBoundaryPatchName,
		    nFaces - defaultPatchStart,
		    defaultPatchStart,
		    boundary.size() - 1,
		    boundary
		)
	    );
	    
	    nAllPatches++;
	}
	
	// Reset the size of the boundary
	boundary.setSize(nAllPatches);
	
	// Apply the boundaries to the instance of fvMesh
	List< polyPatch* > pPolyPatchList( boundary.size() );
	forAll( boundary, patchI )
	{
	    const polyPatch& patch = boundary[ patchI ];
	    autoPtr< polyPatch > polyPatchPtr = patch.clone( mesh->boundaryMesh() );
	    pPolyPatchList[ patchI ] = polyPatchPtr.ptr();
	}
	
	mesh->addFvPatches( pPolyPatchList );
	
	return fvMeshPtr( mesh );
    }
}


// ************************************************************************* //
