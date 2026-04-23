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
// Created: Feb 16, 2026
// Revision: A
//
// Description: This file contains the implementation of the
// ModTakeda class.


#include <ModTakeda.h>
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

static int numModTakedaMaterials = 0;

void *
OPS_ModTakeda()
{
  // Pointer to a uniaxial material that will be returned
  UniaxialMaterial *theMaterial = 0;

  int    iData[1];
  double dData[5];
  int numData = 1;

  // Print information
  if (numModTakedaMaterials == 0) {
      opserr << "ModTakeda unaxial material - Written by Wenchen Lie (Feb 16, 2026)\n";
      numModTakedaMaterials++;
  }

  if (OPS_GetIntInput(&numData, iData) != 0) {
      opserr << "WARNING invalid uniaxialMaterial ModTakeda tag" << endln;
      return 0;
  }

  numData = OPS_GetNumRemainingInputArgs();
  int num = 5;
  const char* error_msg1 = "Invalid #args, want: uniaxialMaterial ModTakeda ";
  const char* error_msg2 = " Fy k0 r alpha beta";

  if (numData != 5) {
      opserr << error_msg1 << iData[0] << error_msg2 << endln;
      return 0;
  }

  numData = 5;
  if (OPS_GetDoubleInput(&numData, dData) != 0) {
      opserr << error_msg1 << iData[0] << error_msg2 << endln;
      return 0;
  }

  int tag = iData[0];
  double Fy, k0, r, alpha, beta;
  Fy = dData[0];
  k0 = dData[1];
  r = dData[2];
  alpha = dData[3];
  beta = dData[4];

  if (Fy < 0) {
      opserr << "WARNING Fy must be non-negative" << endln;
      return 0;
  }
  if (k0 < 0) {
      opserr << "WARNING k0 must be non-negative" << endln;
      return 0;
  }
  if (r < 0) {
      opserr << "WARNING r must be non-negative" << endln;
      return 0;
  }
  if (alpha < 0) {
      opserr << "WARNING alpha must be positive" << endln;
      return 0;
  }
  if (beta < 0) {
      opserr << "WARNING beta must be positive" << endln;
      return 0;
  }

  //opserr << "Fy = " << Fy << "\n" << endln;
  //opserr << "k0 = " << k0 << "\n" << endln;
  //opserr << "r = " << r << "\n" << endln;
  //opserr << "alpha = " << alpha << "\n" << endln;
  //opserr << "beta = " << beta << "\n" << endln;

  // Parsing was successful, allocate the material
  theMaterial = new ModTakeda(tag, Fy, k0, r, alpha, beta);
  if (theMaterial == 0) {
    opserr << "WARNING could not create uniaxialMaterial of type ModTakeda Material" << endln;
    return 0;
  }

  return theMaterial;
}



ModTakeda::ModTakeda(int tag_, double Fy_, double k0_, double r_, double alpha_, double beta_)
    :UniaxialMaterial(tag_, MAT_TAG_ModTakeda),
    Fy(Fy_), k0(k0_), r(r_), alpha(alpha_), beta(beta_)
{
    Cstrain = 0.0;
    Cstress = 0.0;
    Tstrain = 0.0;
    Tstress = 0.0;
    Ttangent = k0;
    Ctangent = k0;
    uy = Fy / k0;
    Cdm_pos = Fy / k0;
    Tdm_pos = Fy / k0;
    Cdm_neg = -Fy / k0;
    Tdm_neg = -Fy / k0;
    CFm_pos = Fy;
    TFm_pos = Fy;
    CFm_neg = -Fy;
    TFm_neg = -Fy;
}

ModTakeda::ModTakeda():UniaxialMaterial(0, MAT_TAG_ModTakeda),
    Fy(0.0), k0(0.0), r(0.0), alpha(0.0), beta(0.0),
    uy (0.0),
    Cstrain(0.0), Tstrain(0.0), Cstress(0.0), Tstress(0.0), Ctangent(0.0), Ttangent(0.0),
    Cdm_pos(0.0), Tdm_pos(0.0), Cdm_neg(0.0), Tdm_neg(0.0), CFm_pos(0.0), TFm_pos(0.0), CFm_neg(0.0), TFm_neg(0.0)
{

}

ModTakeda::~ModTakeda ()
{
    
}

int ModTakeda::setTrialStrain (double strain, double strainRate)
{
    double dStrain = strain - Cstrain;
    Tstrain = strain;
    if (fabs(dStrain) <= DBL_EPSILON) {
        return 0;
    }
    if (dStrain > 0) {
        double u_flag = fmax(uy, Cdm_pos - beta * (Cdm_pos - uy));
        double F_flag = fmax(Fy, CFm_pos - beta * (Cdm_pos - uy) * r * k0);
        if (Cstress < 0) {
            double ku = k0 * pow(fabs(uy / Cdm_pos), alpha);
            if (ku < fabs((Cstress + DBL_EPSILON) / (Cstrain + DBL_EPSILON))) {
                ku = fabs((Cstress + DBL_EPSILON) / (Cstrain + DBL_EPSILON));
            }
            if (Cstress + ku * dStrain > 0) {
                double dStrain1 = -Cstress / ku;
                double dStrain2 = dStrain - dStrain1;
                double u0 = Cstrain + dStrain1;
                double kr;
                if (strain < u_flag) {
                    kr = F_flag / (u_flag - u0);
                }
                else {
                    kr = k0;
                }
                Tstress = kr * dStrain2;
            }
            else {
                Tstress = Cstress + ku * dStrain;
            }
        }
        else {
            if (u_flag > strain) {
                double kr = (F_flag - Cstress) / (u_flag - Cstrain);
                Tstress = Cstress + kr * dStrain;
            }
            else {
                Tstress = Cstress + dStrain * k0;
            }
        }
        if (Tstress > r * k0 * (strain - uy) + Fy) {
            Tstress = r * k0 * (strain - uy) + Fy;
        }
    }
    else {
        double u_flag = fmin(-uy, Cdm_neg - beta * (Cdm_neg + uy));
        double F_flag = fmin(-Fy, CFm_neg - beta * (Cdm_neg + uy) * r * k0);
        if (Cstress > 0) {
            double ku = k0 * pow(abs(uy / Cdm_neg), alpha);
            if (ku < fabs((Cstress + DBL_EPSILON) / (Cstrain + DBL_EPSILON))) {
                ku = fabs((Cstress + DBL_EPSILON) / (Cstrain + DBL_EPSILON));
            }
            if (Cstress + ku * dStrain < 0) {
                double dStrain1 = -Cstress / ku;
                double dStrain2 = dStrain - dStrain1;
                double u0 = Cstrain + dStrain1;
                double kr;
                if (strain > u_flag) {
                    kr = F_flag / (u_flag - u0);
                }
                else {
                    kr = k0;
                }
                Tstress = kr * dStrain2;
            }
            else {
                Tstress = Cstress + ku * dStrain;
            }
        }
        else {
            if (u_flag < strain) {
                double kr = (F_flag - Cstress) / (u_flag - Cstrain);
                Tstress = Cstress + kr * dStrain;
            }
            else {
                Tstress = Cstress + dStrain * k0;
            }
        }
        if (Tstress < r * k0 * (strain + uy) - Fy) {
            Tstress = r * k0 * (strain + uy) - Fy;
        }
    }
    // Update flag point
    if (dStrain > 0) {
        Tdm_pos = fmax(Tdm_pos, Tstrain);
        TFm_pos = fmax(TFm_pos, Tstress);
    }
    else {
        Tdm_neg = fmin(Tdm_neg, Tstrain);
        TFm_neg = fmin(TFm_neg, Tstress);
    }
    Ttangent = (Tstress - Cstress) / dStrain;
    return 0;
}

double ModTakeda::getStrain ()
{
    return Tstrain;
}

double ModTakeda::getStress ()
{
    return Tstress;
}

double ModTakeda::getTangent ()
{
    return Ttangent;
}

double ModTakeda::getInitialTangent()
{
    return Fy / k0;
}

int ModTakeda::commitState ()
{
    Cstrain = Tstrain;
    Cstress = Tstress;
    Ctangent = Ttangent;
    Cdm_pos = Tdm_pos;
    Cdm_neg = Tdm_neg;
    CFm_pos = TFm_pos;
    CFm_neg = TFm_neg;
    return 0;
}

int ModTakeda::revertToLastCommit ()
{
    Cstrain = Tstrain;
    Cstress = Tstress;
    Ctangent = Ttangent;
    Cdm_pos = Tdm_pos;
    Cdm_neg = Tdm_neg;
    CFm_pos = TFm_pos;
    CFm_neg = TFm_neg;
    return 0;
}



int ModTakeda::revertToStart ()
{
   Cstrain = 0.0;
   Cstress = 0.0;
   Ttangent = Fy / k0;
   Ctangent = Fy / k0;
   Tstrain = 0.0;
   Tstress = 0.0;
   Cdm_pos = 0.0;
   Tdm_pos = 0.0;
   Cdm_neg = 0.0;
   Tdm_neg = 0.0;
   CFm_pos = 0.0;
   TFm_pos = 0.0;
   CFm_neg = 0.0;
   TFm_neg = 0.0;
   return 0;
}

UniaxialMaterial* ModTakeda::getCopy ()
{
   ModTakeda* theCopy = new ModTakeda(this->getTag(), Fy, k0, r, alpha, beta);
   return theCopy;
}

int ModTakeda::sendSelf (int commitTag, Channel& theChannel)
{
    opserr << "Currently ModTakeda::sendSelf() is not avaiable for ModTakeda material" << endln;
    return -1;
}

int ModTakeda::recvSelf (int commitTag, Channel& theChannel,
                                FEM_ObjectBroker& theBroker)
{
    opserr << "Currently ModTakeda::recvSelf() is not avaiable for ModTakeda material" << endln;
    return -1;

}

void ModTakeda::Print (OPS_Stream& s, int flag)
{
  if (flag == OPS_PRINT_PRINTMODEL_MATERIAL) {    
    s << "ModTakeda tag:   " << this->getTag() << endln;
    s << "  Fy:          " << Fy << " ";
    s << "  k0:          " << k0 << " ";
    s << "  r:           " << r << " ";
    s << "  alpha:       " << alpha << " ";
    s << "  beta:        " << beta << " ";
  }

  if (flag == OPS_PRINT_PRINTMODEL_JSON) {
    s << "\t\t\t{";
	s << "\"tag\": \"" << this->getTag() << "\", ";
    s << "\"type\": \"ModTakeda\", ";
    s << "\"Fy\": " << Fy << ", ";
    s << "\"k0\": " << k0 << ", ";
    s << "\"r\": " << r << ", ";
	s << "\"alpha " << alpha << ", ";
    s << "\"beta\": " << beta << ", ";
  }
  
}
