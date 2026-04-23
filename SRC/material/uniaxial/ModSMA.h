// ######## MoWen ##########

#ifndef ModSMA_h
#define ModSMA_h


#include <UniaxialMaterial.h>
//#include <Matrix.h>

class ModSMA : public UniaxialMaterial
{
public:
    ModSMA(int tag, double k1, double k2, double k3, double k4, double k5, double k6, double k7,
        double C1, double C2, double C3, double C6, double C7, double beta);
    ModSMA();
    ~ModSMA();

    const char* getClassType(void) const { return "ModSMA"; };

    int setTrialStrain(double strain, double strainRate = 0.0);
    double getStrain(void);
    double getStress(void);
    double getTangent(void);
    double getInitialTangent(void) { return k1; };

    int commitState(void);
    int revertToLastCommit(void);
    int revertToStart(void);

    UniaxialMaterial* getCopy(void);

    int sendSelf(int commitTag, Channel& theChannel);
    int recvSelf(int commitTag, Channel& theChannel,
        FEM_ObjectBroker& theBroker);

    void Print(OPS_Stream& s, int flag = 0);

protected:

private:

    //试验状态变量
    double Tstrain;
    double Tstress;
    double Ttangent;
    double Cstrain;
    double Cstress;
    double Ctangent;

    //刚度
    double k1;
    double k2;     //第二段的刚度
    double k3;
    double k4;
    double k5;
    double lk5;
    double k6;     //第六段的刚度
    double lk6;    //实时的第六段的刚度
    double k7;     //第七段的刚度
    double lk7;    //实时的第七段的刚度
    double k8;
    double k382;   //第三段卸载时，在第八段再加载时，第二段临时刚度
    double k482;

    //原始输入拐点
    double C1;
    double C2;
    double C3;
    double C6;
    double C7;

    //
    double beta;
    double Cbeta;
    double remain;   //残余变形

    //
    double Rstrain;

    //可变化的拐点
    double C1Pos;
    double C1Neg;
    double C2Pos;
    double C2Neg;
    double C3Pos;
    double C3Neg;
    double C6Pos;
    double C6Neg;
    double BC6;
    double BC6Neg;
    double C7Pos;
    double C7Neg;
    double BC7;
    double BC7Neg;

    //用于判断的拐点
    double Cu2;
    double Cu2Neg;
    double Cu3;
    double Cu3Neg;
    double Cu4;
    double Cu4Neg;

    //每段结束的应力和应变
    double STs1;
    double STs2;
    double SST2;

    //用于记录实时应力
    double Ts1;
    double Ts2;
    double Ts3;
    double Ts4;

    //用于记录实时应变
    double ST1;
    double ST2;
    double ST3;
    double ST4;

    //卸载拐点
    double Xu2;
    double Xu2Neg;
    double Xu3;
    double Xu3Neg;
    double Xu31;    //用于区分在第三段卸载时的卸载第一段
    double XuNeg31;
    double Xu4;
    double Xu4Neg;
    double Xu41;
    double XuNeg41;

    //卸载段的力和位移
    double Cu2strain;
    double ZCu2strain;
    double Cu2stress;
    double ZCu2stress;
    double Cu3strain;
    double ZCu3strain;
    double Cu3stress;
    double ZCu3stress;
    double Cu36strain;    //在第三段卸载时，第六段的卸载的力
    double ZCu36strain;
    double Cu36stress;
    double ZCu36stress;
    double Cu37strain;    //在第三段卸载时，第七段的卸载的力
    double ZCu37strain;
    double Cu37stress;
    double ZCu37stress;
    double Cu38strain;    //在第三段卸载时，第八段的卸载的力
    double ZCu38strain;
    double Cu38stress;
    double ZCu38stress;
    double ZCu39stress;
    double ZCu39strain;
    double Cu4strain;
    double ZCu4strain;
    double Cu4stress;
    double ZCu4stress;
    double Cu46strain;    //在第四段卸载时，第六段的卸载的力
    double ZCu46strain;
    double Cu46stress;
    double ZCu46stress;
    double Cu47strain;    //在第四段卸载时，第七段的卸载的力
    double ZCu47strain;
    double Cu47stress;
    double ZCu47stress;
    double Cu48strain;    //在第四段卸载时，第八段的卸载的力
    double ZCu48strain;
    double Cu48stress;
    double ZCu48stress;
    double ZCu49stress;
    double ZCu49strain;

    //用于判断加/卸载段
    double Ca;
    double CaNeg;
    double load;
    double loadPos;
    double loadNeg;
    double Xu;
    double XuNeg;

    //用于判断再加载
    double ZCu2;
    double TCu2;
    double ZCu3;
    double TCu3;
    double TCu36;
    double TCu362;    //在第三段卸载时，第六段的第二种情况
    double TCu37;
    double TCu372;
    double TCu38;
    double ZCu4;
    double ZCu43;
    double ZCu43Neg;
    double ZCu44;
    double TCu4;
    double TCu46;
    double TCu462;
    double TCu47;
    double TCu472;
    double TCu473;
    double TCu48;
    double TCu482;
    double ZCu5;

    //
    double segment;
    double i;
    double j;
    double n;
    double m;

    //
    double mode;
    double Res_deformation_Pos;
    double Res_deformation_Neg;
    double qremain;
    double Trange;
    double Dvalue;
    double nostress;
    double nostrain;
    double reload;
    double TBC;

    double Fk1;
    double Fk2;     //第二段的刚度
    double Fk3;
    double Fk4;
    double Fk5;
    double Fk6;     //第六段的刚度
    double Fk7;     //第七段的刚度

    double Track_L;   //应力-应变的变化转化为力和位移的系数（长度）
    double Track_K;   //应力-应变的变化转化为力和位移的系数（刚度）

    double Fc3;
    double Fc5;
    double Fc6;
    double Fc7;
}


#endif
;