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
                                                                        
// $Revision: 1.1 $
// $Date: 2026-02-22 12:00:00 $
// $Source: /usr/local/cvs/OpenSees/SRC/material/uniaxial/ModTakeda.h,v $
                                                                        
                                                                        
#ifndef ModTakeda_h
#define ModTakeda_h

// Written: Wenchen Lie 
// Created: Feb 16, 2026
// Revision: A
//
// Description: This file contains the class definition for 
// ModTakeda.h
// 
//
//
// What: "@(#) ModTakeda.h, revA"


#include <UniaxialMaterial.h>

class ModTakeda : public UniaxialMaterial
{
  public:
    ModTakeda(int tag, double Fy, double k0, double r, double alpha, double beta);
    ModTakeda();
    ~ModTakeda();

    const char *getClassType(void) const {return "ModTakeda";};

    int setTrialStrain(double strain, double strainRate = 0.0); 
    double getStrain(void);              
    double getStress(void);
    double getTangent(void);
    double getInitialTangent(void);

    int commitState(void);
    int revertToLastCommit(void);    
    int revertToStart(void);        

    UniaxialMaterial *getCopy(void);
    
    int sendSelf(int commitTag, Channel &theChannel);  
    int recvSelf(int commitTag, Channel &theChannel, 
		 FEM_ObjectBroker &theBroker);    
    
    void Print(OPS_Stream &s, int flag =0);
    

 protected:
    
 private:
	/*** Material Properties ***/
    double Fy;
    double k0;
    double r;
    double alpha;
    double beta;

    /*** State variables ***/
    double Cstrain;
    double Tstrain;
    double Cstress;
    double Tstress;
    double Ctangent;
    double Ttangent;
    double uy;
    double Cdm_pos;  // Max. history strain
    double Tdm_pos;
    double Cdm_neg;  // Min. history strain
    double Tdm_neg;
    double CFm_pos;  // Max. history stress
    double TFm_pos;
    double CFm_neg;  // Min. history stress
    double TFm_neg;

};

#endif
