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
// $Source: /usr/local/cvs/OpenSees/SRC/material/uniaxial/Degradation.h,v $
                                                      
// Written: Wenchen Lie
// Created: Jan 22, 2025
//
// Description: This file contains the class definition for Degradation.h

#ifndef Degradation_h
#define Degradation_h

#include <UniaxialMaterial.h>

class Degradation : public UniaxialMaterial
{
  public:
    Degradation(int tag, UniaxialMaterial &material, double uy, double uTh,
        double a1, double a2); 
    Degradation();
    ~Degradation();
    
    const char *getClassType(void) const {return "Degradation";};

    int setTrialStrain(double strain, double strainRate = 0.0); 
    double getStrain(void);          
    double getStrainRate(void);
    double getStress(void);
    double getTangent(void);
    double getDampTangent(void);
    double getInitialTangent(void) {return theMaterial->getInitialTangent();}

    int commitState(void);
    int revertToLastCommit(void);    
    int revertToStart(void);        

    UniaxialMaterial *getCopy(void);
    
    int sendSelf(int commitTag, Channel &theChannel);  
    int recvSelf(int commitTag, Channel &theChannel, 
		 FEM_ObjectBroker &theBroker);    
    
    void Print(OPS_Stream &s, int flag =0);
    bool hasFailed(void) {return false;}

  protected:
    
  private:
	UniaxialMaterial *theMaterial;

    double uy;
    double uTh;
    double a1;
    double a2;

    bool Cstate;
    bool Tstate;

    double Cyieldface;
    double Tyieldface;
    double Cuc;
    double Tuc;
    double Cuc2;
    double Tuc2;
    double Cumax;
    double Tumax;
    double Cstrain;
    double Tstrain;
    double CstrainRate;
    double TstrainRate;
    double Cstress;
    double Tstress;
    double Ctangent;
    double Ttangent;
    double Cdegradation;
    double Tdegradation;
};


#endif

