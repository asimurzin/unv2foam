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
    I-Deas unv format mesh conversion.

    Uses either
    - DOF sets (757) or
    - face groups (2452(Cubit), 2467)
    to do patching.
    Works without but then puts all faces in defaultFaces patch.

\*---------------------------------------------------------------------------*/

#include "unv2foam.H"

#include "argList.H"

#include "fvMesh.H"

using namespace Foam;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
dictionary createControlDict()
{
  // creating dummy controlDict for correct initialisation autoPtr<fileMonitor> in runTime
  dictionary a_controlDict;
  a_controlDict.add( word( "startFrom" ), word( "startTime" ) );
  a_controlDict.add( word( "startTime" ), 0.0 );

  a_controlDict.add( word( "stopAt" ), word( "endTime" ) );
  a_controlDict.add( word( "endTime" ), 0.0 );

  a_controlDict.add( word( "deltaT" ), 0.0 );
  a_controlDict.add( word( "writeControl" ), word( "timeStep" ) );

  a_controlDict.add( word( "writeInterval" ), 1 );
  
  return a_controlDict;
}
// Main program:

int main(int argc, char *argv[])
{
    argList::noParallel();
    argList::validArgs.append(".unv file");

#   include "setRootCase.H"

//#   include "createTime.H"
    // To avoid unnecessary reading from file Foam::Time::controlDictName
    // the creation of the Time object is expanded into different way
    Foam::Info<< "Create time\n" << Foam::endl;

    dictionary a_controlDict = createControlDict();
    Foam::Time runTime
    (
        a_controlDict,
        args.rootPath(),
        args.caseName()
    );

#if ( __FOAM_VERSION__ >= 010500 )
    fileName ideasName( args.additionalArgs()[0] );
#else
    fileName ideasName( args.params()[2] );
#endif
    fvMeshPtr mesh = Foam::unv2foam( ideasName, runTime );

    mesh->write();

    return 0;
}


// ************************************************************************* //
