/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
                                                                        
// $Revision: 1.0 $
// $Date: 2025-01-22 13:50:00 $
// $Source: /usr/local/cvs/OpenSees/SRC/material/uniaxial/Degradation.cpp,v $

// Written: Wenchen Lie
// Created: Jan 22, 2025
//
// Description: This file contains the class definition for Degradation.h

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Degradation.h>
#include <ID.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>

#include <OPS_Globals.h>

#include <elementAPI.h>
#define OPS_Export 

static int numDegradationMaterials = 0;

OPS_Export void *
OPS_Degradation(void)
{
  // Print information
  if (numDegradationMaterials == 0) {
      opserr << "Degradation unaxial wrapper material - Written by Wenchen Lie (Jan 22, 2025)\n";
      numDegradationMaterials++;
  }

  // Pointer to a uniaxial material that will be returned
  UniaxialMaterial *theMaterial = 0;
  UniaxialMaterial *theOtherMaterial = 0;
  double dData[4];
  int    iData[2];

  int argc = OPS_GetNumRemainingInputArgs();
  if (argc < 6) {
    opserr << "WARNING invalid uniaxialMaterial Degradation $tag $otherTag $uy $u_th $a1 $a2" << endln;
    return 0;
  }

  int numData = 2;
  if (OPS_GetIntInput(&numData, iData) != 0) {
    opserr << "WARNING invalid uniaxialMaterial Degradation $tag $otherTag" << endln;
    return 0;
  }

  theOtherMaterial = OPS_GetUniaxialMaterial(iData[1]);
  if (theOtherMaterial == 0) {
    opserr << "WARNING invalid otherTag uniaxialMaterial Degradation tag: " << iData[0] << endln;
    return 0;	
  }

  numData = 4;
  if (OPS_GetDouble(&numData, dData) != 0) {
    opserr << "WARNING invalid double values for uniaxialMaterial Degradation tag: " << iData[0] << "\n$uy $u_th $a1 $a2" << endln;
    return 0;
  }

  if (dData[0] < 0.0 || dData[1] <= 0.0 || dData[2] < 0.0 || dData[3] < 0.0) {
    opserr << "WARNING invalid parameters for uniaxialMaterial Degradation tag: " << iData[0]
           << "\nuy, a1, and a2 must be >= 0, and u_th must be > 0" << endln;
    return 0;
  }

  // Parsing was successful, allocate the material
  theMaterial = new Degradation(iData[0], *theOtherMaterial, dData[0], dData[1], dData[2], dData[3]);

  if (theMaterial == 0) {
    opserr << "WARNING could not create uniaxialMaterial of type Degradation\n";
    return 0;
  }

  return theMaterial;
}

Degradation::Degradation(int tag, UniaxialMaterial &material, double uy, double uTh,
    double a1, double a2)
  :UniaxialMaterial(tag,MAT_TAG_Degradation), theMaterial(0),
   uy(uy), uTh(uTh), a1(a1), a2(a2),
   Cstate(false), Tstate(false),
   Cyieldface(uy), Tyieldface(uy),
   Cuc(0.0), Tuc(0.0), Cuc2(0.0), Tuc2(0.0), Cumax(0.0), Tumax(0.0),
   Cstrain(0.0), Tstrain(0.0), CstrainRate(0.0), TstrainRate(0.0),
   Cstress(0.0), Tstress(0.0), Ctangent(0.0), Ttangent(0.0),
   Cdegradation(1.0), Tdegradation(1.0)
{
  theMaterial = material.getCopy();

  if (theMaterial == 0) {
    opserr <<  "Degradation::Degradation -- failed to get copy of material\n";
    exit(-1);
  }
}

Degradation::Degradation()
  :UniaxialMaterial(0,MAT_TAG_Degradation), theMaterial(0),
   uy(0.0), uTh(0.0), a1(0.0), a2(0.0),
   Cstate(false), Tstate(false),
   Cyieldface(0.0), Tyieldface(0.0),
   Cuc(0.0), Tuc(0.0), Cuc2(0.0), Tuc2(0.0), Cumax(0.0), Tumax(0.0),
   Cstrain(0.0), Tstrain(0.0), CstrainRate(0.0), TstrainRate(0.0),
   Cstress(0.0), Tstress(0.0), Ctangent(0.0), Ttangent(0.0),
   Cdegradation(1.0), Tdegradation(1.0)
{

}

Degradation::~Degradation()
{
  if (theMaterial)
    delete theMaterial;
}

int Degradation::setTrialStrain(double strain, double strainRate)
{
  Tstate = Cstate;
  Tyieldface = Cyieldface;
  Tuc = Cuc;
  Tuc2 = Cuc2;
  Tumax = Cumax;
  Tstrain = Cstrain;
  TstrainRate = CstrainRate;
  Tstress = Cstress;
  Ttangent = Ctangent;
  Tdegradation = Cdegradation;

  double dStrain = strain - Cstrain;
  if (dStrain == 0.0)
    return 0;

  int res = theMaterial->setTrialStrain(strain, strainRate);

  Tstrain = strain;
  TstrainRate = strainRate;

  double absStrain = fabs(Tstrain);
  if (absStrain > Tumax)
    Tumax = absStrain;

  if (Cstate)
    Tstate = true;
  else if (absStrain >= uTh)
    Tstate = true;
  else
    Tstate = false;

  if (strain > Cyieldface)
  {
    // Positive yielding
    Tuc = Cuc + strain - Cyieldface;
    Tyieldface = strain;
    if (Tstate)
      Tuc2 = Cuc2 + strain - Cyieldface;
  }
  else if (strain < Cyieldface - 2 * uy)
  {
    // Negative yielding
    Tuc = Cuc + Cyieldface - 2 * uy - strain;
    Tyieldface = strain + 2 * uy;
    if (Tstate)
      Tuc2 = Cuc2 + Cyieldface - 2 * uy - strain;
  }

  double lambda1 = 1.0;
  if (Tstate) {
    lambda1 = 1.0 - a1 * Tuc2 / uTh;
    if (lambda1 < 0.0)
      lambda1 = 0.0;
  }

  double lambda2 = 1.0;
  if (Tstate && Tuc2 > 0.0) {
    lambda2 = 1.0 - a2 * (Tuc - Tuc2) / Tuc2;
    if (lambda2 < 0.0)
      lambda2 = 0.0;
  }

  Tdegradation = lambda1 < lambda2 ? lambda1 : lambda2;
  Tstress = theMaterial->getStress() * Tdegradation;
  Ttangent = theMaterial->getTangent() * Tdegradation;

  return res;
}

double Degradation::getStress(void)
{
  return Tstress;
}

double Degradation::getTangent(void)
{
  return Ttangent;
}

double Degradation::getDampTangent(void)
{
  return theMaterial->getDampTangent() * Tdegradation;
}

double Degradation::getStrain(void)
{
  return Tstrain;
}

double Degradation::getStrainRate(void)
{
  return TstrainRate;
}

int Degradation::commitState(void)
{	
  Cstate = Tstate;
  Cyieldface = Tyieldface;
  Cuc = Tuc;
  Cuc2 = Tuc2;
  Cumax = Tumax;
  Cstrain = Tstrain;
  CstrainRate = TstrainRate;
  Cstress = Tstress;
  Ctangent = Ttangent;
  Cdegradation = Tdegradation;

  return theMaterial->commitState();
}

int Degradation::revertToLastCommit(void)
{
  Tstate = Cstate;
  Tyieldface = Cyieldface;
  Tuc = Cuc;
  Tuc2 = Cuc2;
  Tumax = Cumax;
  Tstrain = Cstrain;
  TstrainRate = CstrainRate;
  Tstress = Cstress;
  Ttangent = Ctangent;
  Tdegradation = Cdegradation;

  return theMaterial->revertToLastCommit();
}

int Degradation::revertToStart(void)
{
  Cstate = false;
  Tstate = false;
  Cyieldface = uy;
  Tyieldface = uy;
  Cuc = 0.0;
  Tuc = 0.0;
  Cuc2 = 0.0;
  Tuc2 = 0.0;
  Cumax = 0.0;
  Tumax = 0.0;
  Cstrain = 0.0;
  Tstrain = 0.0;
  CstrainRate = 0.0;
  TstrainRate = 0.0;
  Cstress = 0.0;
  Tstress = 0.0;
  Ctangent = 0.0;
  Ttangent = 0.0;
  Cdegradation = 1.0;
  Tdegradation = 1.0;

  return theMaterial->revertToStart();
}

UniaxialMaterial *
Degradation::getCopy(void)
{
  Degradation *theCopy = 
    new Degradation(this->getTag(), *theMaterial, uy, uTh, a1, a2);
        
  theCopy->Cyieldface = Cyieldface;
  theCopy->Tyieldface = Tyieldface;
  theCopy->Cstate = Cstate;
  theCopy->Tstate = Tstate;
  theCopy->Cuc = Cuc;
  theCopy->Tuc = Tuc;
  theCopy->Cuc2 = Cuc2;
  theCopy->Tuc2 = Tuc2;
  theCopy->Cumax = Cumax;
  theCopy->Tumax = Tumax;
  theCopy->Cstrain = Cstrain;
  theCopy->Tstrain = Tstrain;
  theCopy->CstrainRate = CstrainRate;
  theCopy->TstrainRate = TstrainRate;
  theCopy->Cstress = Cstress;
  theCopy->Tstress = Tstress;
  theCopy->Ctangent = Ctangent;
  theCopy->Ttangent = Ttangent;
  theCopy->Cdegradation = Cdegradation;
  theCopy->Tdegradation = Tdegradation;
  
  return theCopy;
}

int Degradation::sendSelf(int cTag, Channel &theChannel)
{
    opserr << "Degradation::sendSelf() is not avaiable for Degradation material" << endln;
    return -1;
}

int Degradation::recvSelf(int cTag, Channel &theChannel, 
			 FEM_ObjectBroker &theBroker)
{
    opserr << "Degradation::recvSelf() is not avaiable for Degradation material" << endln;
    return -1;
}

void Degradation::Print(OPS_Stream &s, int flag)
{
    if (flag == OPS_PRINT_PRINTMODEL_MATERIAL) {
        s << "Degradation, tag: " << this->getTag() << endln;
        s << "  material: " << theMaterial->getTag() << endln;
        s << "  uy        : " << uy << endln;
        s << "  u_th      : " << uTh << endln;
        s << "  a1        : " << a1 << endln;
        s << "  a2        : " << a2 << endln;
    }
    
    if (flag == OPS_PRINT_PRINTMODEL_JSON) {
        s << "\t\t\t{";
        s << "\"name\": \"" << this->getTag() << "\", ";
        s << "\"type\": \"Degradation\", ";
        s << "\"material\": \"" << theMaterial->getTag() << "\", ";
        s << "\"uy\": " << uy << ", ";
        s << "\"u_th\": " << uTh << ", ";
        s << "\"a1\": " << a1 << ", ";
        s << "\"a2\": " << a2 << "}";
    }
}
