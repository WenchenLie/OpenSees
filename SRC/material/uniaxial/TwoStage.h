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
// $Date: 2025-05-30 23:17:00 $
// $Source: /usr/local/cvs/OpenSees/SRC/material/uniaxial/TwoStage.h,v $
                                                                        
                                                                        
#ifndef TwoStage_h
#define TwoStage_h

// Written: Wenchen Lie 
// Created: May 30, 2025
// Revision: A
//
// Description: This file contains the class definition for 
// TwoStage.h
// 
//
//
// What: "@(#) TwoStage.h, revA"


#include <UniaxialMaterial.h>

class TwoStage : public UniaxialMaterial
{
  public:
    TwoStage(int tag, double F1, double k1, double k1p, double F2, double k2, double k2p, double ua);
    TwoStage();
    ~TwoStage();

    const char *getClassType(void) const {return "TwoStage";};

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
    double F1;
    double k1;
    double kp1;
    double F2;
    double k2;
    double kp2;
    double ua;

    /*** State variables ***/
    double Cstrain;
    double Tstrain;
    double Cstrain2;
    double Tstrain2;
    double Cstress;
    double Tstress;
    double Cstress1;
    double Tstress1;
    double Cstress2;
    double Tstress2;
    double Ctangent;
    double Ttangent;
    double Chookgap;
    double Thookgap;

    double bilinear(double F_prev, double u_prev, double du, double Fy, double k, double kp);
};

#endif
