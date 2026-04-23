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

  if (F1 <= 0) {
      opserr << "WARNING Fy must be positive" << endln;
      return 0;
  }
  if (k1 <= 0) {
      opserr << "WARNING k1 must be positive" << endln;
      return 0;
  }
  if (kp1 < 0) {
      opserr << "WARNING k1p must be non-negative" << endln;
      return 0;
  }
  if (F2 <= 0) {
      opserr << "WARNING F2 must be positive" << endln;
      return 0;
  }
  if (k2 <= 0) {
      opserr << "WARNING k2 must be positive" << endln;
      return 0;
  }
  if (kp2 < 0) {
      opserr << "WARNING k2p must be non-negative" << endln;
      return 0;
  }
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
    Cstress = 0.0;
    Tstrain = 0.0;
    Tstress = 0.0;
    Ttangent = k1_;
    Ctangent = k1_;
    Cstage = 1;
    Tstage = 1;
    Cu_pos = ua;
    Tu_pos = ua;
    Cu_neg = -ua;
    Tu_neg = -ua;
}

TwoStage::TwoStage():UniaxialMaterial(0, MAT_TAG_TwoStage),
    F1(0.0), k1(0.0), kp1(0.0), F2(0.0), k2(0.0), kp2(0.0), ua(0.0),
    Cstrain(0.0), Tstrain(0.0), Cstress(0.0), Tstress(0.0), Ctangent(0.0), Ttangent(0.0),
    Cstage(1), Tstage(1), Cu_pos(0.0), Tu_pos(0.0), Cu_neg(0.0), Tu_neg(0.0)
{

}

TwoStage::~TwoStage ()
{
    
}

int TwoStage::setTrialStrain (double strain, double strainRate)
{
   double dStrain = strain - Cstrain;
   Tstrain = strain;
    if (fabs(dStrain) <= DBL_EPSILON) {
        return 0;
    }
    // Determine stage
    if (Tstrain > Cu_pos) {
        Tstage = 2;
        Tu_pos = Tstrain;
        Tu_neg = Tu_pos - 2 * ua;
    }
    else if (Tstrain < Cu_neg) {
        Tstage = 2;
        Tu_neg = Tstrain;
        Tu_pos = Tu_neg + 2 * ua;
    }
    else {
        Tstage = 1;
    }
    // Calculate stress && tangent
    if (Cstage == 1 && Tstage == 1) {
        // Remain stage - 1
        Tstress = bilinear(Cstress, Cstrain, dStrain, F1, k1, kp1);
    }
    else if (Cstage == 2 && Tstage == 2) {
        // Remain stage - 2
        Tstress = bilinear(Cstress, Cstrain, dStrain, F2, k2, kp2);
    }
    else if (Cstage == 1 && Tstage == 2) {
        // Transition stage - 1 to stage - 2
        double du1;  // Strain component in stage - 1
        double du2;  // Strain component in stage - 2
        double F_trans;  // Trasition stress
        if (dStrain > 0) {
            du2 = Tstrain - Cu_pos;
        }
        else {
            du2 = Tstrain - Cu_neg;
        }
        du1 = dStrain - du2;
        F_trans = bilinear(Cstress, Cstrain, du1, F1, k1, kp1);
        Tstress = bilinear(F_trans, Cstrain + du1, du2, F2, k2, kp2);
    }
    else if (Cstage == 2 && Tstage == 1) {
        // Transition stage - 2 to stage - 1
        double du2;  // Strain component in stage - 2
        double du1;  // Strain component in stage - 1
        double F_trans;  // Trasition stress
        if (dStrain < 0) {
            du1 = Tstrain - Cu_pos;
        }
        else {
            du1 = Tstrain - Cu_neg;
        }
        du2 = dStrain - du1;
        F_trans = bilinear(Cstress, Cstrain, du2, F2, k2, kp2);
        Tstress = bilinear(F_trans, Cstrain + du2, du1, F1, k1, kp1);
    }
    else {
        opserr << "ERROR Should not reach hear (TwoStage Material)" << endln;
    }
    Ttangent = (Tstress - Cstress) / dStrain;
    return 0;
}

double TwoStage::bilinear(double F_prev, double u_prev, double du, double Fy, double k, double kp)
{
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
    return F1 / k1;
}

int TwoStage::commitState ()
{
    Cstrain = Tstrain;
    Cstress = Tstress;
    Ctangent = Ttangent;
    Cu_pos = Tu_pos;
    Cu_neg = Tu_neg;
    Cstage = Tstage;
    return 0;
}

int TwoStage::revertToLastCommit ()
{
    Cstrain = Tstrain;
    Cstress = Tstress;
    Ctangent = Ttangent;
    Cu_pos = Tu_pos;
    Cu_neg = Tu_neg;
    Cstage = Tstage;
    return 0;
}



int TwoStage::revertToStart ()
{
   Cstrain = 0.0;
   Cstress = 0.0;
   Ttangent = F1 / k1;
   Ctangent = F1 / k1;
   Tstrain = 0.0;
   Tstress = 0.0;
   Cu_pos = 0.0;
   Tu_pos = 0.0;
   Cu_neg = 0.0;
   Tu_neg = 0.0;
   Cstage = 1;
   Tstage = 1;
   return 0;
}

UniaxialMaterial* TwoStage::getCopy ()
{
   TwoStage* theCopy = new TwoStage(this->getTag(), F1, k1, kp1, F2, k2, kp2, ua);
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
