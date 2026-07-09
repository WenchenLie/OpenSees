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
** file 'COPYRIGHT'  in main directory for information on usage &&   **
** redistribution,  && for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
                                                                        
// Written: Wenchen Lie (666@e.gzhu.edu.cn)
// Created: May 30, 2025
// Revision: A
//
// Description: This file contains the implementation of the
// TwoStage class.


#include <TwoStage.h>
#include <Vector.h>
#include <Matrix.h>
#include <Channel.h>
#include <Information.h>
#include <Parameter.h>

#include <string.h>

#include <math.h>
#include <float.h>


#include <elementAPI.h>
#include <OPS_Globals.h>

static int numTwoStageMaterials = 0;

void *
OPS_TwoStage()
{
  // Pointer to a uniaxial material that will be returned
  UniaxialMaterial *theMaterial = 0;

  int    iData[1];
  double dData[7];
  int numData = 1;

  // Print information
  if (numTwoStageMaterials == 0) {
      opserr << "TwoStage unaxial material - Written by Wenchen Lie (May 30, 2024)\n";
      numTwoStageMaterials++;
  }

  if (OPS_GetIntInput(&numData, iData) != 0) {
      opserr << "WARNING invalid uniaxialMaterial TwoStage tag" << endln;
      return 0;
  }

  numData = OPS_GetNumRemainingInputArgs();
  int num = 7;
  const char* error_msg1 = "Invalid #args, want: uniaxialMaterial TwoStage ";
  const char* error_msg2 = " F1 k1 kp1 F2 k2 kp2 ua";

  if (numData != 7) {
      opserr << error_msg1 << iData[0] << error_msg2 << endln;
      return 0;
  }

  numData = 7;
  if (OPS_GetDoubleInput(&numData, dData) != 0) {
      opserr << error_msg1 << iData[0] << error_msg2 << endln;
      return 0;
  }

  int tag = iData[0];
  double F1, k1, kp1, F2, k2, kp2, ua;
  F1 = dData[0];
  k1 = dData[1];
  kp1 = dData[2];
  F2 = dData[3];
  k2 = dData[4];
  kp2 = dData[5];
  ua = dData[6];

  if (ua < 0) {
      opserr << "WARNING ua must be non-negative" << endln;
      return 0;
  }

  //opserr << "F1 = " << F1 << "\n" << endln;
  //opserr << "k1 = " << k1 << "\n" << endln;
  //opserr << "kp1 = " << kp1 << "\n" << endln;
  //opserr << "F2 = " << F2 << "\n" << endln;
  //opserr << "k2 = " << k2 << "\n" << endln;
  //opserr << "kp2 = " << kp2 << "\n" << endln;
  //opserr << "ua = " << ua << "\n" << endln;

  // Parsing was successful, allocate the material
  theMaterial = new TwoStage(tag, F1, k1, kp1, F2, k2, kp2, ua);
  if (theMaterial == 0) {
    opserr << "WARNING could not create uniaxialMaterial of type TwoStage Material" << endln;
    return 0;
  }

  return theMaterial;
}



TwoStage::TwoStage(int tag_, double F1_, double k1_, double kp1_, double F2_, double k2_, double kp2_, double ua_)
    :UniaxialMaterial(tag_, MAT_TAG_TwoStage),
    F1(F1_), k1(k1_), kp1(kp1_), F2(F2_), k2(k2_), kp2(kp2_), ua(ua_)
{
    Cstrain = 0.0;
    Tstrain = 0.0;
    Cstrain2 = 0.0;
    Tstrain2 = 0.0;
    Cstress = 0.0;
    Tstress = 0.0;
    Cstress1 = 0.0;
    Tstress1 = 0.0;
    Cstress2 = 0.0;
    Tstress2 = 0.0;
    Ttangent = k1_;
    Ctangent = k1_;
    Chookgap = 0.0;
    Thookgap = 0.0;
}

TwoStage::TwoStage():UniaxialMaterial(0, MAT_TAG_TwoStage),
    F1(0.0), k1(0.0), kp1(0.0), F2(0.0), k2(0.0), kp2(0.0), ua(0.0),
    Cstrain(0.0), Tstrain(0.0), Cstrain2(0.0), Tstrain2(0.0),
    Cstress(0.0), Tstress(0.0), Cstress1(0.0), Tstress1(0.0),
    Cstress2(0.0), Tstress2(0.0), Ctangent(0.0), Ttangent(0.0),
    Chookgap(0.0), Thookgap(0.0)
{

}

TwoStage::~TwoStage ()
{
    
}

int TwoStage::setTrialStrain (double strain, double strainRate)
{
    double dstrain2 = 0.0;
    double dStrain = strain - Cstrain;

    Tstrain = strain;
    Tstrain2 = Cstrain2;
    Tstress = Cstress;
    Tstress1 = Cstress1;
    Tstress2 = Cstress2;
    Ttangent = Ctangent;
    Thookgap = Chookgap;

    if (fabs(dStrain) <= DBL_EPSILON) {
        return 0;
    }

    Tstress1 = bilinear(Cstress1, Cstrain, dStrain, F1, k1, kp1);

    if (-ua < Thookgap && Thookgap < ua) {
        if (dStrain > 0) {
            double tmp = Thookgap + dStrain - ua;
            dstrain2 = tmp > 0.0 ? tmp : 0.0;
            Thookgap = Thookgap + dStrain < ua ? Thookgap + dStrain : ua;
        }
        else {
            double tmp = Thookgap + dStrain + ua;
            dstrain2 = tmp < 0.0 ? tmp : 0.0;
            Thookgap = Thookgap + dStrain > -ua ? Thookgap + dStrain : -ua;
        }
        Tstress2 = bilinear(Cstress2, Cstrain2, dstrain2, F2, k2, kp2);
        Tstrain2 = Cstrain2 + dstrain2;
    }
    else if (Thookgap == ua) {
        Tstress2 = bilinear(Cstress2, Cstrain2, dStrain, F2, k2, kp2);
        Tstrain2 = Cstrain2 + dStrain;
        if (dStrain <= 0.0 && Tstress2 < 0.0) {
            double denom = fabs(Tstress2) + fabs(Cstress2);
            if (denom > DBL_EPSILON) {
                dstrain2 = dStrain * fabs(Cstress2) / denom;
                Thookgap = ua + (dStrain - dstrain2);
                if (Thookgap < -ua) {
                    Thookgap = -ua;
                }
            }
            Tstress2 = 0.0;
        }
    }
    else if (Thookgap == -ua) {
        Tstress2 = bilinear(Cstress2, Cstrain2, dStrain, F2, k2, kp2);
        Tstrain2 = Cstrain2 + dStrain;
        if (dStrain >= 0.0 && Tstress2 > 0.0) {
            double denom = fabs(Tstress2) + fabs(Cstress2);
            if (denom > DBL_EPSILON) {
                dstrain2 = dStrain * fabs(Cstress2) / denom;
                Thookgap = -ua + (dStrain - dstrain2);
                if (Thookgap > ua) {
                    Thookgap = ua;
                }
            }
            Tstress2 = 0.0;
        }
    }
    else {
        opserr << "ERROR Should not reach here (TwoStage Material, Thookgap = "
               << Thookgap << ", ua = " << ua << ")" << endln;
        return -1;
    }

    Tstress = Tstress1 + Tstress2;
    Ttangent = (Tstress - Cstress) / dStrain;
    return 0;
}

double TwoStage::bilinear(double F_prev, double u_prev, double du, double Fy, double k, double kp)
{
    if (Fy == 0.0) {
        return 0.0;
    }
    double F_next = F_prev + du * k;
    if (F_next > kp * (u_prev + du) + (1 - kp / k) * Fy && du > 0) {
        F_next = kp * (u_prev + du) + (1 - kp / k) * Fy;
    }
    else if (F_next < kp * (u_prev + du) - (1 - kp / k) * Fy && du < 0) {
        F_next = kp * (u_prev + du) - (1 - kp / k) * Fy;
    }
    return F_next;
}

double TwoStage::getStrain ()
{
    return Tstrain;
}

double TwoStage::getStress ()
{
    return Tstress;
}

double TwoStage::getTangent ()
{
    return Ttangent;
}

double TwoStage::getInitialTangent()
{
    return k1;
}

int TwoStage::commitState ()
{
    Cstrain = Tstrain;
    Cstrain2 = Tstrain2;
    Cstress = Tstress;
    Cstress1 = Tstress1;
    Cstress2 = Tstress2;
    Ctangent = Ttangent;
    Chookgap = Thookgap;
    return 0;
}

int TwoStage::revertToLastCommit ()
{
    Tstrain = Cstrain;
    Tstrain2 = Cstrain2;
    Tstress = Cstress;
    Tstress1 = Cstress1;
    Tstress2 = Cstress2;
    Ttangent = Ctangent;
    Thookgap = Chookgap;
    return 0;
}



int TwoStage::revertToStart ()
{
   Cstrain = 0.0;
   Tstrain = 0.0;
   Cstrain2 = 0.0;
   Tstrain2 = 0.0;
   Cstress = 0.0;
   Tstress = 0.0;
   Cstress1 = 0.0;
   Tstress1 = 0.0;
   Cstress2 = 0.0;
   Tstress2 = 0.0;
   Ttangent = k1;
   Ctangent = k1;
   Chookgap = 0.0;
   Thookgap = 0.0;
   return 0;
}

UniaxialMaterial* TwoStage::getCopy ()
{
   TwoStage* theCopy = new TwoStage(this->getTag(), F1, k1, kp1, F2, k2, kp2, ua);
   theCopy->Cstrain = Cstrain;
   theCopy->Tstrain = Tstrain;
   theCopy->Cstrain2 = Cstrain2;
   theCopy->Tstrain2 = Tstrain2;
   theCopy->Cstress = Cstress;
   theCopy->Tstress = Tstress;
   theCopy->Cstress1 = Cstress1;
   theCopy->Tstress1 = Tstress1;
   theCopy->Cstress2 = Cstress2;
   theCopy->Tstress2 = Tstress2;
   theCopy->Ctangent = Ctangent;
   theCopy->Ttangent = Ttangent;
   theCopy->Chookgap = Chookgap;
   theCopy->Thookgap = Thookgap;
   return theCopy;
}

int TwoStage::sendSelf (int commitTag, Channel& theChannel)
{
    opserr << "Currently TwoStage::sendSelf() is not avaiable for TwoStage material" << endln;
    return -1;
}

int TwoStage::recvSelf (int commitTag, Channel& theChannel,
                                FEM_ObjectBroker& theBroker)
{
    opserr << "Currently TwoStage::recvSelf() is not avaiable for TwoStage material" << endln;
    return -1;

}

void TwoStage::Print (OPS_Stream& s, int flag)
{
  if (flag == OPS_PRINT_PRINTMODEL_MATERIAL) {    
    s << "TwoStage tag:   " << this->getTag() << endln;
    s << "  F1:          " << F1 << " ";
    s << "  k1:          " << k1 << " ";
    s << "  kp1:         " << kp1 << " ";
    s << "  F2:          " << F2 << " ";
    s << "  k2:          " << k2 << " ";
    s << "  kp2:         " << kp2 << " ";
    s << "  ua:          " << ua << " ";
  }

  if (flag == OPS_PRINT_PRINTMODEL_JSON) {
    s << "\t\t\t{";
	s << "\"tag\": \"" << this->getTag() << "\", ";
    s << "\"type\": \"TwoStage\", ";
    s << "\"F1\": " << F1 << ", ";
    s << "\"k1\": " << k1 << ", ";
    s << "\"kp1\": " << kp1 << ", ";
	s << "\"F2 " << F2 << ", ";
    s << "\"k2\": " << k2 << ", ";
    s << "\"kp2\": " << kp2 << ", ";
    s << "\"ua\": " << ua << ", ";
  }
  
}
