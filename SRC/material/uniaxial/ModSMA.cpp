

#include <ModSMA.h>
#include <Vector.h>
#include <Channel.h>
#include <Matrix.h>
#include <Information.h>
#include <Parameter.h>

#include <math.h>
#include <float.h>
#include <elementAPI.h>


void* OPS_ModSMA()
{
	int numdata = OPS_GetNumRemainingInputArgs();
	if (numdata < 12) {
		opserr << "WARNING: Insufficient arguements\n";
		opserr << "Want: uniaxialMaterial ModSMA tag? k1? k3? ";
		opserr << "ActF? beta? <SlipDef? BearDef? rBear?>" << endln;
		return 0;
	}

	int tag;
	numdata = 1;
	if (OPS_GetIntInput(&numdata, &tag) < 0) {
		opserr << "WARNING invalid tag\n";
		return 0;
	}

	double data[13] = { 0,0,0,0,0,0,0,0,0,0,0,0,0 };
	numdata = OPS_GetNumRemainingInputArgs();
	if (numdata > 13) {
		numdata = 13;
	}
	if (OPS_GetDoubleInput(&numdata, data)) {
		opserr << "WARNING invalid double inputs\n";
		return 0;
	}

	UniaxialMaterial* mat = new ModSMA(tag, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12]);
	if (mat == 0) {
		opserr << "WARNING: failed to create ModSMA material\n";
		return 0;
	}

	return mat;
}

ModSMA::ModSMA(int tag, double K1, double K2, double K3, double K4, double K5, double K6, double K7,
	double c1, double c2, double c3,
	double c6, double c7, double b)
	: UniaxialMaterial(tag, MAT_TAG_ModSMA),
	k1(K1), k2(K2), k3(K3), k4(K4), k5(K5), k6(K6), k7(K7), C1(c1), C2(c2), C3(c3), C6(c6), C7(c7), beta(b)
{

	Ca = 0;
	CaNeg = 0;
	remain = 0;
	qremain = 0;
	i = 0;
	j = 0;
	n = 0;
	m = 0;
	load = 0;

	// 初始化变量
	this->revertToStart();

}

ModSMA::ModSMA()
	: UniaxialMaterial(0, MAT_TAG_ModSMA),
	k1(0.0), k2(0.0), k3(0.0), k4(0.0), k5(0.0), k6(0.0), k7(0.0), C1(0.0), C2(0.0), C3(0.0), C6(0.0), C7(0.0), beta(0.0)
{


	//Ca = 0;

	this->revertToStart();

}

ModSMA::~ModSMA()
{

}

int
ModSMA::setTrialStrain(double strain, double strainRate)
{
	Tstrain = strain;
	//
	// 分析模型，mode=1:当位移和力的正负相反时，令力为零，
	//           mode=2:当位移和力的正负相反时，不令力为零
	mode = 1;
	//
	Track_L = 1;
	Track_K = 1;
	//
	//qremain = Track_L * (-0.09878 + 0.4206 * log(n + 1)) * 3;
	if (Trange == 1) {
		if (n == 0 || n == 1) {
			remain = 0;
		}
	}
	if (m == n) {
		m = m + 1;
		if (n == 0) {
			C1Pos = C1;
			C2Pos = C2;
			C3Pos = C3;
			C6Pos = C6;
			C7Pos = C7;
			BC6 = C6;
			BC7 = C7;
			BC6Neg = -C6;
			BC7Neg = -C7;
			Fk1 = k1;
			k2 = k2;
			Fk3 = k3;
			k4 = k4;
			lk5 = k5;
			lk6 = k6;
			lk7 = k7;
			Cbeta = beta;

		}
	}

	if (Tstrain >= 0) {
		loadNeg = 0;
		Rstrain = Tstrain;
		if ((Rstrain <= C3) && (Ca == 0)) {
			Cbeta = beta * Rstrain / C3;
		}
		else if ((Rstrain > C3) && (Ca == 0)) {
			Fc3 = C1 * k1 + k2 * (C2 - C1) + k3 * (C3 - C2);
			Fc5 = Fc3 - k5 * beta;
			Fc6 = Fc5 - k6 * (C3 - beta - C6);
			Fc7 = Fc6 - k7 * (C6 - C7);
		}
		if ((Rstrain <= C1Pos * k1 / Fk1) && (Ca == 0) && (load == 0)) {//加载段第一段
			if (mode == 1) {
				Tstress = Fk1 * (Rstrain - remain);
			}
			else if (mode == 2) {
				Tstress = Res_deformation_Neg + Fk1 * Tstrain;
			}
			Ttangent = Fk1;
			Ts1 = Tstress;
			STs1 = Tstress;
			ST1 = Rstrain;
			segment = 1;
			//
			Cu2 = 0;    //用于第二段判断卸载
			if (i == j) {  //判断循环圈数
				n = n + 1;
				i = i + 1;
			}
		}
		else if ((Rstrain <= C2Pos) && (Rstrain >= Cu2) && (Ca == 0) && (load == 0)) {//加载段第二段
			Tstress = Ts1 + (Rstrain - ST1) * k2;
			Ttangent = k2;
			Ts2 = Tstress;
			STs2 = Tstress;
			ST2 = Rstrain;
			SST2 = Rstrain;
			segment = 2;
			Xu2 = (Rstrain - C1Pos) * 0.5 + C1Pos;   //当在第二段卸载时，卸载第一段的长度
			Cu2 = Rstrain;      //用于第二段判断卸载
			Cu3 = Rstrain;      //用于第三段判断卸载
			Xu = 0;
			if (i == j) {  //判断循环圈数
				n = n + 1;
				i = i + 1;
			}
			Trange = 0;
			Res_deformation_Neg = 0;
		}
		else if ((Rstrain <= C3Pos) && (Rstrain >= Cu3) && (Ca == 0)) {//加载段第三段
			Tstress = Ts2 + (Rstrain - ST2) * Fk3;
			Ttangent = Fk3;
			Ts3 = Tstress;
			ST3 = Rstrain;
			segment = 3;

			//拟合数据时，需要修改的地方          1
			Xu3 = Rstrain - Cbeta;    //当在第三段卸载时，卸载第一段的长度
			//Xu3 = (0.79 / beta) * (-0.253 + 1.023 * Rstrain - 0.00987 * pow(Rstrain, 2));   //当在第三段卸载时，卸载第一段的长度

			Cu3 = Rstrain;      //用于第三段判断卸载
			Cu4 = Rstrain;
			Xu = 0;
			Xu31 = 0;
			if (i == j) {  //判断循环圈数
				n = n + 1;
				i = i + 1;
			}
			Trange = 0;
		}
		else if ((Rstrain > C3Pos) && (Rstrain >= Cu4) && (Ca == 0)) {//加载段第四段
			Tstress = Ts3 + (Rstrain - ST3) * k4;
			Ttangent = k4;
			Ts4 = Tstress;
			ST4 = Rstrain;
			segment = 4;
			Cbeta = (Tstress - Fc5) / k5;
			//拟合数据时，需要修改的地方 
			Xu4 = Rstrain - Cbeta;
			//Xu4 = (0.79 / beta) * (-0.253 + 1.023 * Rstrain - 0.00987 * pow(Rstrain, 2));    //当在第四段卸载时，卸载第一段的长度

			Xu41 = 0;
			Cu4 = Rstrain;
			Xu = 0;
		}
		else if ((Rstrain <= Cu2) && (Xu == 0) && (segment == 2)) {  //当从第二段开始卸载时
			if (Rstrain >= Xu2) {//卸载第一段
				Ca = 1;
				Tstress = Ts2 + (Rstrain - ST2) * Fk1;
				Ttangent = Fk1;
				Cu2stress = Tstress;
				Cu2strain = Rstrain;
				ZCu2 = Rstrain;   //用于判断再加载
				k8 = Cu2stress / (Cu2strain - qremain);
				TCu2 = Ts1 + (Rstrain - ST1) * k2;
				if (Tstress >= (TCu2)) {
					Ca = 0;
					load = 0;
					Cu2 = 0;
					Ts1 = Tstress;   //于第二段相连接
					ST1 = Rstrain;   //于第二段相连接
				}
			}
			else {
				if (Rstrain <= ZCu2) {//卸载第二段
					load = 1;
					Tstress = Cu2stress + (Rstrain - Cu2strain) * k8;
					Ttangent = k8;
					ZCu2 = Rstrain;
					ZCu2strain = Rstrain;
					ZCu2stress = Tstress;
					//if (i != j) {  //判断圈数
					//	j = j + 1;
					//}
				}
				else {//再加载段
					Tstress = ZCu2stress + (Rstrain - ZCu2strain) * Fk1;
					Ttangent = Fk1;
					Xu2 = 10 * C2Pos;    //再加载时，不允许卸载第一段
					TCu2 = Ts1 + (Rstrain - ST1) * k2;
					if (Tstress >= (TCu2 - 0.5) && (Rstrain >= C1Pos)) {
						Ca = 0;
						load = 0;
						Cu2 = 0;
						Ts1 = Tstress;   //于第二段相连接
						ST1 = Rstrain;   //于第二段相连接
					}
				}
			}
		}
		else if ((Rstrain <= Cu3) && (Xu == 0) && (segment == 3)) {//当从第三段开始卸载时
			if ((Rstrain >= Xu3) && (Xu31 == 0)) {//卸载第一段
				Ca = 1;
				load = 1;
				Tstress = Ts3 + (Rstrain - ST3) * lk5;
				Ttangent = lk5;
				Cu3stress = Tstress;
				Cu3strain = Rstrain;
				ZCu3 = Rstrain;   //用于判断再加载
				//拟合的时候，需要修改的地方    3
				BC6 = C1 + (ST3 - C1) / (C3 - C1) * (C6 - C1);
				TCu3 = Ts2 + (Rstrain - ST2) * Fk3;
				TBC = 1;
				if (Tstress >= (TCu3)) {
					Ca = 0;
					load = 0;
					Cu3 = 0;
					Ts2 = Tstress;   //于第三段相连接
					ST2 = Rstrain;   //于第三段相连接
				}
			}
			else if (BC6 > Xu3) {
				BC6 = Xu3;
				Cu36stress = Tstress;
				Cu36strain = Rstrain;
				ZCu36stress = Tstress;
				ZCu36strain = Rstrain;
				ZCu3 = Rstrain;   //用于判断再加载
				ZCu4 = Rstrain;
				BC7 = C1 + (ST3 - C1) / (C3 - C1) * (C7 - C1);
			}
			else if ((Rstrain < Xu3) && (Rstrain >= BC6)) {
				if (Rstrain <= ZCu3) {//卸载第二段
					Tstress = Cu3stress + (Rstrain - Cu3strain) * lk6;
					Ttangent = lk6;
					Cu36stress = Tstress;
					Cu36strain = Rstrain;
					ZCu36stress = Tstress;
					ZCu36strain = Rstrain;
					ZCu3 = Rstrain;   //用于判断再加载
					ZCu4 = Rstrain;
					//拟合的时候，需要修改的地方      4
					BC7 = C1 + (ST3 - C1) / (C3 - C1) * (C7 - C1);
					load = 1;
					TBC = 1;
					if (i != j) {  //判断圈数
						j = j + 1;
					}
				}
				else {//卸载第二段再加载
					Tstress = ZCu36stress + (Rstrain - ZCu36strain) * lk5;
					Ttangent = lk5;
					Xu3 = 1000 * C2Pos;    //再加载时，不允许卸载第一段
					TCu36 = Ts2 + (Rstrain - ST2) * Fk3;  //情况一
					TCu362 = Ts1 + (Rstrain - ST1) * k2;  //情况二
					if ((Tstress >= (TCu36 - 0.1)) || (Tstress >= (TCu362 - 0.1))) {    //当力大于，当前位移下按照第二段或者第三段计算的力
						if (Tstress >= (TCu36 - 0.1)) {
							Ca = 0;
							load = 0;
							Cu3 = 0;
							Ts2 = Tstress;   //于第三段相连接
							ST2 = Rstrain;   //于第三段相连接
						}
						if (Tstress >= (TCu362 - 0.1)) {
							Ca = 0;
							load = 0;
							Cu2 = 0;
							Ts1 = Tstress;   //于第三段相连接
							ST1 = Rstrain;   //于第三段相连接
						}
					}
				}
			}
			else if ((Rstrain < BC6) && (Rstrain >= BC7)) {//卸载段第三段
				if (Rstrain <= ZCu4) {
					Tstress = Cu36stress + (Rstrain - Cu36strain) * lk7;
					Ttangent = lk7;
					Cu37stress = Tstress;
					Cu37strain = Rstrain;
					ZCu37stress = Tstress;
					ZCu37strain = Rstrain;
					ZCu3 = Rstrain;
					ZCu4 = Rstrain;
					ZCu5 = Rstrain;
					k8 = Cu37stress / (Cu37strain - qremain);
					TBC = 1;
				}
				else {//卸载第三段再加载
					BC6 = Cu3;
					Tstress = ZCu37stress + (Rstrain - ZCu37strain) * Fk1;
					Ttangent = Fk1;
					Xu31 = 1;
					TCu37 = Ts2 + (Rstrain - ST2) * Fk3;  //情况一
					TCu372 = Ts1 + (Rstrain - ST1) * k2;  //情况二
					if ((Tstress >= (TCu37 - 0.05)) || (Tstress >= (TCu372 - 0.05))) {    //当力大于，当前位移下按照第二段或者第三段计算的力
						if (Tstress >= (TCu37 - 0.05)) {
							Ca = 0;
							load = 0;
							Cu3 = 0;
							Ts2 = Tstress;   //于第三段相连接
							ST2 = Rstrain;   //于第三段相连接
						}
						if (Tstress >= (TCu372 - 0.05)) {
							Ca = 0;
							load = 0;
							Cu2 = 0;
							Ts1 = Tstress;   //于第三段相连接
							ST1 = Rstrain;   //于第三段相连接
						}
					}
				}
			}
			//特殊情况，当BC7>BC6时，这不运行卸载段第三段
			else if ((BC7 > BC6) && (TBC == 1)) {
				BC7 = BC6;
				//定义卸载第四段需要的参数
				Cu37stress = Cu36stress;
				Cu37strain = Cu36strain;
				ZCu37stress = ZCu36stress;
				ZCu37strain = ZCu36strain;
				ZCu3 = Cu36strain;
				ZCu4 = Cu36strain;
				ZCu5 = Cu36strain;
				k8 = Cu36stress / (Cu36strain - qremain);
				TBC = 0;
			}
			else if (Rstrain < BC7) {//卸载段第四段
				if (Rstrain <= ZCu5) {
					Tstress = Cu37stress + (Rstrain - Cu37strain) * k8;
					Ttangent = k8;
					ZCu38stress = Tstress;
					ZCu38strain = Rstrain;
					ZCu39stress = Tstress;
					ZCu39strain = Rstrain;
					ZCu5 = Rstrain;
					k382 = (STs2 - ZCu38stress) / (SST2 - ZCu38strain);
					TBC = 0;
				}
				else {//卸载第四段再加载
					if (Tstress <= STs1) {//再加载第一段
						Tstress = ZCu38stress + (Rstrain - ZCu38strain) * Fk1;
						Ttangent = Fk1;
						ZCu39stress = Tstress;
						ZCu39strain = Rstrain;
						k382 = (STs2 - ZCu39stress) / (SST2 - ZCu39strain);
						BC6 = 0;
						Xu3 = 0;
						Xu31 = 1;
						BC7 = ST3;
					}
					else {//再加载第二段
						BC6 = 0;
						Xu3 = 0;
						Xu31 = 1;
						BC7 = ST3;
						TBC = 0;
						if (Rstrain <= C2Pos) {//情况一
							Tstress = ZCu39stress + (Rstrain - ZCu39strain) * k382;
							Ttangent = k382;
							TCu37 = Ts1 + (Rstrain - ST1) * k2;
							if (Tstress >= (TCu37 - 0.1)) {
								Ca = 0;
								load = 0;
								Cu3 = 0;
								Cu2 = 0;
								Ts2 = Tstress;   //于第三段相连接
								ST2 = Rstrain;   //于第三段相连接
							}
						}
						else {//情况二
							Tstress = ZCu39stress + (Rstrain - ZCu39strain) * k2;
							Ttangent = k2;
							TCu37 = Ts2 + (Rstrain - ST2) * Fk3;
							if (Tstress >= (TCu37 - 0.1)) {
								Ca = 0;
								load = 0;
								Cu3 = 0;
								Cu2 = 0;
								Ts2 = Tstress;   //于第三段相连接
								ST2 = Rstrain;   //于第三段相连接
							}
						}
					}
				}
			}
		}
		else if ((Rstrain <= Cu4) && (Xu == 0) && (segment == 4)) {//当从第四段开始卸载时
			if ((Rstrain >= Xu4) && (Xu41 == 0)) {//卸载第一段
				Ca = 1;
				load = 1;
				Tstress = Ts4 + (Rstrain - ST4) * lk5;
				Ttangent = lk5;
				Cu4stress = Tstress;
				Cu4strain = Rstrain;
				ZCu4 = Rstrain;
				Cbeta = (Ts4 - Fc5) / k5;
				BC6 = ST4 - Cbeta - (C3 - beta - C6);
				//
				TCu3 = Ts3 + (Rstrain - ST3) * k4;
				Cu3 = 1000000;      //Cu3无限大
				if (Tstress >= (TCu3)) {
					Ca = 0;
					load = 0;
					Cu3 = 0;
					Cu4 = 0;
					Ts3 = Tstress;   //于第三段相连接
					ST3 = Rstrain;   //于第三段相连接
				}
				if (Rstrain < BC6) {
					BC7 = C1 + (ST4 - C1) / (C3 - C1) * (C7 - C1);
					Cu46stress = Tstress;
					Cu46strain = Rstrain;
					ZCu46stress = Tstress;
					ZCu46strain = Rstrain;
					ZCu4 = Rstrain;
					ZCu43 = Rstrain;
				}
			}
			else if (BC6 > Xu4) {
				BC6 = Xu4;
				Cu46stress = Tstress;
				Cu46strain = Rstrain;
				ZCu46stress = Tstress;
				ZCu46strain = Rstrain;
				ZCu4 = Rstrain;
				ZCu43 = Rstrain;
				//拟合的时候，需要修改的地方      6
				BC7 = (Fc7 - Fc6) / k7 + BC6;
			}
			else if ((Rstrain < Xu4) && (Rstrain >= BC6)) {
				if (Rstrain <= ZCu4) {//卸载段第二段
					Tstress = Cu4stress + (Rstrain - Cu4strain) * lk6;
					Ttangent = lk6;
					Cu46stress = Tstress;
					Cu46strain = Rstrain;
					ZCu46stress = Tstress;
					ZCu46strain = Rstrain;
					ZCu4 = Rstrain;
					ZCu43 = Rstrain;
					//拟合的时候，需要修改的地方      6
					BC7 = (Fc7 - Fc6) / k7 + BC6;
					//BC7 = C1 + (ST4 - C1) / (C3 - C1) * (C7 - C1);
					//BC7 = C7 * (0.76 + 0.5 * log(ST3)) / 1.89;
					//lk7 = (k7 * (46.283 - 13.404 * ST3 + 1.23 * pow(ST3, 2)) / 7.6);
					//BC7 = Track_L * (0.2777 * ST4 + 0.1248);
					////BC7 = Track_L * (qremain + 1.5);
					//lk7 = Track_K * (2.289 * pow(ST4, 2) - 43.34 * ST4 + 239.9);
					//
					if (i != j) {  //判断圈数
						j = j + 1;
					}
				}
				else {//卸载第二段的位置再加载
					Tstress = ZCu46stress + (Rstrain - ZCu46strain) * lk5;
					Ttangent = lk5;
					Xu4 = 10 * C3Pos;
					TCu46 = Ts2 + (Rstrain - ST2) * Fk3;  //情况一
					TCu462 = Ts3 + (Rstrain - ST3) * k4;  //情况二
					if ((Tstress >= (TCu46 - 0.1)) || (Tstress >= (TCu462 - 0.1))) {    //当力大于，当前位移下按照第二段或者第三段计算的力
						if (Tstress >= (TCu46 - 0.1) && (Rstrain <= C3Pos)) {
							Ca = 0;
							load = 0;
							Cu3 = 0;
							Ts2 = Tstress;   //于第三段相连接
							ST2 = Rstrain;   //于第三段相连接
						}
						if (Tstress >= (TCu462 - 0.1) && (Rstrain >= C3Pos)) {
							Ca = 0;
							load = 0;
							Cu4 = 0;
							Ts3 = Tstress;   //于第三段相连接
							ST3 = Rstrain;   //于第三段相连接
						}
					}
				}
			}
			else if ((Rstrain < BC6) && (Rstrain >= BC7)) {//卸载段第三段
				if (Rstrain <= ZCu43) {
					Tstress = Cu46stress + (Rstrain - Cu46strain) * lk7;
					Ttangent = lk7;
					Cu47stress = Tstress;
					Cu47strain = Rstrain;
					ZCu47stress = Tstress;
					ZCu47strain = Rstrain;
					ZCu4 = Rstrain;
					ZCu43 = Rstrain;
					ZCu44 = Rstrain;
					k8 = Cu47stress / Cu47strain;
				}
				else {//在卸载第三段的位置再加载
					BC6 = Cu4;
					Tstress = ZCu47stress + (Rstrain - ZCu47strain) * Fk1;
					Ttangent = Fk1;
					Xu41 = 1;
					TCu47 = Ts2 + (Rstrain - ST2) * Fk3;    //情况一
					TCu472 = Ts1 + (Rstrain - ST1) * k2;   //情况二
					TCu473 = Ts3 + (Rstrain - ST3) * k4;   //情况三
					if ((Tstress >= (TCu47 - 0.1)) || (Tstress >= (TCu472 - 0.1)) || (Tstress >= (TCu473 - 0.1))) {    //当力大于，当前位移下按照第二段或者第三段计算的力
						if (Tstress >= (TCu47 - 0.1)) {
							Ca = 0;
							load = 0;
							Cu3 = 0;
							Ts2 = Tstress;   //于第三段相连接
							ST2 = Rstrain;   //于第三段相连接
						}
						if (Tstress >= (TCu472 - 0.1)) {
							Ca = 0;
							load = 0;
							Cu2 = 0;
							Ts1 = Tstress;   //于第三段相连接
							ST1 = Rstrain;   //于第三段相连接
						}
						if (Tstress >= (TCu473 - 0.1) && (Rstrain >= C3Pos)) {
							Ca = 0;
							load = 0;
							Cu4 = 0;
							Ts3 = Tstress;   //于第三段相连接
							ST3 = Rstrain;   //于第三段相连接
						}
					}
				}
			}
			//else if (BC7 > BC6) {
			//	BC7 = BC6;
			//	Cu47stress = Tstress;
			//	Cu47strain = Rstrain;
			//	ZCu47stress = Tstress;
			//	ZCu47strain = Rstrain;
			//	ZCu4 = Rstrain;
			//	ZCu43 = Rstrain;
			//	ZCu44 = Rstrain;
			//	k8 = Cu47stress / (Cu47strain);
			//}
			else if (Rstrain < BC7) {//卸载第四段
				if (Rstrain <= ZCu44) {
					Tstress = Cu47stress + (Rstrain - Cu47strain) * k8;
					Ttangent = k8;
					ZCu48stress = Tstress;
					ZCu48strain = Rstrain;
					ZCu44 = Rstrain;
					ZCu49stress = Tstress;
					ZCu49strain = Rstrain;
					k482 = (STs2 - ZCu49stress) / (SST2 - ZCu49strain);
				}
				else {//在卸载第四段的位置再加载
					if (Tstress <= STs1) {//再加载第一段
						Tstress = ZCu48stress + (Rstrain - ZCu48strain) * Fk1;
						Ttangent = Fk1;
						ZCu49stress = Tstress;
						ZCu49strain = Rstrain;
						k482 = (STs2 - ZCu49stress) / (SST2 - ZCu49strain);
						BC6 = 0;
						Xu4 = 0;
						Xu41 = 1;
						BC7 = ST3;
					}
					else {//再加载第二段
						BC6 = 0;
						Xu4 = 0;
						Xu41 = 1;
						BC7 = ST3;
						if (Rstrain <= C2Pos) {//情况一
							Tstress = ZCu49stress + (Rstrain - ZCu49strain) * k482;
							Ttangent = k482;
							TCu47 = Ts1 + (Rstrain - ST1) * k2;
							if (Tstress >= (TCu47 - 0.1)) {
								Ca = 0;
								load = 0;
								Cu3 = 0;
								Cu2 = 0;
								Cu4 = 0;
								Ts2 = Tstress;   //于第三段相连接
								ST2 = Rstrain;   //于第三段相连接
							}
						}
						else {//情况二
							Tstress = ZCu49stress + (Rstrain - ZCu49strain) * k2;
							Ttangent = k2;
							TCu47 = Ts2 + (Rstrain - ST2) * Fk3;
							if (Tstress >= (TCu47 - 0.1)) {
								Ca = 0;
								load = 0;
								Cu3 = 0;
								Cu2 = 0;
								Cu4 = 0;
								Ts2 = Tstress;   //于第三段相连接
								ST2 = Rstrain;   //于第三段相连接
							}
						}
					}
				}
			}
		}
		if ((Tstrain <= 0.00001) && (mode == 1)) {
			Ca = 0;
			Xu = 1;
			load = 0;
		}
		if (Tstress <= 0) {
			if (mode == 1) {
				Tstress = 0;
			}
			else if (mode == 2) {
				Res_deformation_Pos = Tstress;
			}
			loadNeg = 0;
			CaNeg = 0;
			XuNeg = 1;
			Trange = 1;
		}
	}
	//
	//
	//
	else {//当位移为负的时候
		Rstrain = Tstrain;
		load = 0;
		if ((Rstrain >= -C3) && (CaNeg == 0)) {
			Cbeta = -beta * Rstrain / C3;
		}
		else if ((Rstrain < -C3) && (CaNeg == 0)) {
			Fc3 = -(C1 * k1 + k2 * (C2 - C1) + k3 * (C3 - C2));
			Fc5 = Fc3 + k5 * beta;
			Fc6 = Fc5 + k6 * (C3 - beta - C6);
			Fc7 = Fc6 + k7 * (C6 - C7);
			//Cbeta = -beta * pow((Rstrain / C3), 3);
		}
		//Cbeta = Track_L * (0.003593 * pow(-Rstrain, 3) - 0.08245 * pow(-Rstrain, 2) + 0.6322 * (-Rstrain) - 0.7822);    // beta和加载应变之间的关系
		//lk5 = Track_K * (-1.089 * pow(-Rstrain, 3) + 25.54 * pow(-Rstrain, 2) - 187.6 * (-Rstrain) + 590.1);
		if ((Rstrain >= C1Neg * k1 / Fk1) && (CaNeg == 0) && (loadNeg == 0)) {//负方向加载段第一段
			if (mode == 1) {
				Tstress = Fk1 * (Rstrain + remain);
			}
			else if (mode == 2) {
				Tstress = Res_deformation_Pos + Fk1 * Tstrain;
			}
			Ttangent = Fk1;
			Ts1 = Tstress;
			STs1 = Tstress;
			ST1 = Rstrain;
			segment = 1;
			//
			Cu2Neg = 0;
			if (i == j) {  //判断循环圈数
				n = n + 1;
				i = i + 1;
			}
		}
		else if ((Rstrain >= C2Neg) && (Rstrain <= Cu2Neg) && (CaNeg == 0) && (loadNeg == 0)) {//负方向加载段第二段
			Tstress = Ts1 + (Rstrain - ST1) * k2;
			Ttangent = k2;
			Ts2 = Tstress;
			STs2 = Tstress;
			ST2 = Rstrain;
			SST2 = Rstrain;
			segment = 2;
			Xu2Neg = (Rstrain - C1Neg) * 0.5 + C1Neg;
			Cu2Neg = Rstrain;
			Cu3Neg = Rstrain;
			XuNeg = 0;
			if (i == j) {  //判断循环圈数
				n = n + 1;
				i = i + 1;
			}
			Trange = 0;
			Res_deformation_Pos = 0;
		}
		else if ((Rstrain >= C3Neg) && (Rstrain <= Cu3Neg) && (CaNeg == 0)) {//负方向加载段第三段
			Tstress = Ts2 + (Rstrain - ST2) * Fk3;
			Ttangent = Fk3;
			Ts3 = Tstress;
			ST3 = Rstrain;
			segment = 3;
			//拟合数据时，需要修改的地方
			//Xu3Neg = (0.79 / beta) * (-(-0.253 + 1.023 * (-Rstrain) - 0.00987 * pow(Rstrain, 2)));
			Xu3Neg = Rstrain + Cbeta;

			Cu3Neg = Rstrain;
			Cu4Neg = Rstrain;
			XuNeg = 0;
			XuNeg31 = 0;
			if (i == j) {  //判断循环圈数
				n = n + 1;
				i = i + 1;
			}
			Trange = 0;
		}
		else if ((Rstrain < C3Neg) && (Rstrain <= Cu4Neg) && (CaNeg == 0)) {//负方向加载段第四段
			Tstress = Ts3 + (Rstrain - ST3) * k4;
			Ttangent = k4;
			Ts4 = Tstress;
			ST4 = Rstrain;
			segment = 4;
			Cbeta = -(Tstress - Fc5) / k5;
			//拟合数据时，需要修改的地方
			//Xu4Neg = (0.79 / beta) * (-(-0.253 + 1.023 * (-Rstrain) - 0.00987 * pow(Rstrain, 2)));   //有效
			Xu4Neg = Rstrain + Cbeta;

			XuNeg41 = 0;
			Cu4Neg = Rstrain;
			XuNeg = 0;
		}
		else if ((Rstrain >= Cu2Neg) && (XuNeg == 0) && (segment == 2)) {//当第二段开始卸载时
			if (Rstrain <= Xu2Neg) {//卸载第一段
				CaNeg = 1;
				Tstress = Ts2 + (Rstrain - ST2) * Fk1;
				Ttangent = Fk1;
				Cu2stress = Tstress;
				Cu2strain = Rstrain;
				ZCu2 = Rstrain;
				k8 = Cu2stress / (Cu2strain + qremain);
				TCu2 = Ts1 + (Rstrain - ST1) * k2;
				if (Tstress <= TCu2) {
					CaNeg = 0;
					loadNeg = 0;
					Cu2Neg = 0;
					Ts1 = Tstress;
					ST1 = Rstrain;
				}
			}
			else {
				if (Rstrain >= ZCu2) {//卸载第二段
					loadNeg = 1;
					Tstress = Cu2stress + (Rstrain - Cu2strain) * k8;
					Ttangent = k8;
					ZCu2 = Rstrain;
					ZCu2strain = Rstrain;
					ZCu2stress = Tstress;
					//if (i != j) {  //判断圈数
					//	j = j + 1;
					//}
				}
				else {//再加载段
					Tstress = ZCu2stress + (Rstrain - ZCu2strain) * Fk1;
					Ttangent = Fk1;
					Xu2Neg = 10 * C2Neg;
					TCu2 = Ts1 + (Rstrain - ST1) * k2;
					if (Tstress <= (TCu2 + 0.1) && (Rstrain <= C1Neg)) {
						CaNeg = 0;
						loadNeg = 0;
						Cu2Neg = 0;
						Ts1 = Tstress;
						ST1 = Rstrain;
					}
				}
			}
		}
		else if ((Rstrain >= Cu3Neg) && (XuNeg == 0) && (segment == 3)) {//当第三段开始卸载时
			if ((Rstrain <= Xu3Neg) && (XuNeg31 == 0)) {//卸载第一段
				CaNeg = 1;
				loadNeg = 1;
				Tstress = Ts3 + (Rstrain - ST3) * lk5;
				Ttangent = lk5;
				Cu3stress = Tstress;
				Cu3strain = Rstrain;
				ZCu3 = Rstrain;
				//拟合数据的时候，需要修改的地方
				BC6Neg = -(C1 + (-ST3 - C1) / (C3 - C1) * (C6 - C1));
				//BC6Neg = Xu3Neg - (C3Neg - C6Neg) * (ST3 / C3Neg);
				//BC6Neg = -(C6 / 4.094) * (-1.91 + 3.59 * log(-ST3));
				//lk6 = (k6 / 21.19) * (47.42 - 10.581 * (-ST3) + 0.998 * pow(ST3, 2));

				//BC6Neg = -Track_L * (0.8871 * (-ST3) - 1.332);
				//lk6 = Track_K * (3.486 * pow(-ST3, 2) - 50.82 * (-ST3) + 257.4);
				//
				TCu3 = Ts2 + (Rstrain - ST2) * Fk3;
				TBC = 1;
				if (Tstress <= TCu3) {
					CaNeg = 0;
					loadNeg = 0;
					Cu3Neg = 0;
					Ts2 = Tstress;
					ST2 = Rstrain;
				}
			}
			else if (BC6Neg < Xu3Neg) {
				BC6Neg = Xu3Neg;
				Cu36stress = Tstress;
				Cu36strain = Rstrain;
				ZCu36stress = Tstress;
				ZCu36strain = Rstrain;
				ZCu3 = Rstrain;
				ZCu4 = Rstrain;
				//拟合数据的时候，需要修改的地方
				BC7Neg = -(C1 + (-ST3 - C1) / (C3 - C1) * (C7 - C1));
			}
			else if ((Rstrain > Xu3Neg) && (Rstrain <= BC6Neg)) {
				if (Rstrain >= ZCu3) {//卸载第二段
					Tstress = Cu3stress + (Rstrain - Cu3strain) * lk6;
					Ttangent = lk6;
					Cu36stress = Tstress;
					Cu36strain = Rstrain;
					ZCu36stress = Tstress;
					ZCu36strain = Rstrain;
					ZCu3 = Rstrain;
					ZCu4 = Rstrain;
					//拟合数据的时候，需要修改的地方
					BC7Neg = -(C1 + (-ST3 - C1) / (C3 - C1) * (C7 - C1));
					//BC7Neg = -(C7 / 1.89) * (0.76 + 0.5 * log(-ST3));
					//lk7 = (k7 / 7.6) * (46.283 - 13.404 * (-ST3) + 1.23 * pow(ST3, 2));
					//BC7Neg = -Track_L * (qremain + 1.5);
					//lk7 = Track_K * (2.289 * pow(-ST3, 2) - 43.34 * (-ST3) + 239.9);
					//BC7Neg = BC6Neg - (C6Neg - C7Neg) * ((ST3 - C2Neg) / C3Neg);
					loadNeg = 1;
					TBC = 1;
					if (i != j) {  //判断圈数
						j = j + 1;
					}
				}
				else {//卸载第二段再加载段
					Tstress = ZCu36stress + (Rstrain - ZCu36strain) * lk5;
					Ttangent = lk5;
					Xu3Neg = 10 * C2Neg;
					TCu36 = Ts2 + (Rstrain - ST2) * Fk3;
					TCu362 = Ts1 + (Rstrain - ST1) * k2;
					if (Tstress <= (TCu36 + 0.1) || (Tstress <= (TCu362 + 0.1))) {
						if (Tstress <= (TCu36 + 0.1)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu3Neg = 0;
							Ts2 = Tstress;
							ST2 = Rstrain;
						}
						if (Tstress <= (TCu362 + 0.1)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu2Neg = 0;
							Ts1 = Tstress;
							ST1 = Rstrain;
						}
					}
				}
			}
			else if ((Rstrain > BC6Neg) && (Rstrain <= BC7Neg)) {
				if (Rstrain >= ZCu4) {//卸载段第三段
					Tstress = Cu36stress + (Rstrain - Cu36strain) * lk7;
					Ttangent = lk7;
					Cu37stress = Tstress;
					Cu37strain = Rstrain;
					ZCu37stress = Tstress;
					ZCu37strain = Rstrain;
					ZCu3 = Rstrain;
					ZCu4 = Rstrain;
					ZCu5 = Rstrain;
					k8 = Cu37stress / (Cu37strain + qremain);
					TBC = 1;
				}
				else {//卸载第三段再加载段
					BC6Neg = Cu3Neg;
					Tstress = ZCu37stress + (Rstrain - ZCu37strain) * Fk1;
					Ttangent = Fk1;
					XuNeg31 = 1;
					TCu37 = Ts2 + (Rstrain - ST2) * Fk3;  //情况一
					TCu372 = Ts1 + (Rstrain - ST1) * k2;  //情况二
					if (Tstress <= (TCu37 + 0.1) || Tstress <= (TCu372 + 0.1)) {
						if (Tstress <= (TCu37 + 0.1)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu3Neg = 0;
							Ts2 = Tstress;
							ST2 = Rstrain;
						}
						if (Tstress <= (TCu372 + 0.1)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu2Neg = 0;
							Ts1 = Tstress;
							ST1 = Rstrain;
						}
					}
				}
			}
			//特殊情况，当BC7Neg<BC6Neg时，令BC7Neg=BC6Neg
			else if ((BC7Neg < BC6Neg) && (TBC == 1)) {
				BC7Neg = BC6Neg;
				Cu37stress = Cu36stress;
				Cu37strain = Cu36strain;
				ZCu37stress = ZCu36stress;
				ZCu37strain = ZCu36strain;
				ZCu3 = Cu36strain;
				ZCu4 = Cu36strain;
				ZCu5 = Cu36strain;
				k8 = Cu36stress / (Cu36strain + qremain);
			}
			else if (Rstrain > BC7Neg) {
				if (Rstrain >= ZCu5) {//卸载段第四段
					Tstress = Cu37stress + (Rstrain - Cu37strain) * k8;
					Ttangent = k8;
					ZCu38stress = Tstress;
					ZCu38strain = Rstrain;
					ZCu39stress = Tstress;  //1
					ZCu39strain = Rstrain;  //1
					ZCu5 = Rstrain;
					k382 = (STs2 - ZCu38stress) / (SST2 - ZCu38strain);  //1
					TBC = 0;
				}
				else {//卸载第四段再加载段
					if (Tstress >= STs1) {//再加载第一段
						Tstress = ZCu38stress + (Rstrain - ZCu38strain) * Fk1;
						Ttangent = Fk1;
						ZCu39stress = Tstress;
						ZCu39strain = Rstrain;
						k382 = (STs2 - ZCu39stress) / (SST2 - ZCu39strain);
						BC6Neg = 0;
						Xu3Neg = 0;
						XuNeg31 = 1;
						BC7Neg = ST3;
					}
					else {//再加载第二段
						BC6Neg = 0;
						Xu3Neg = 0;
						XuNeg31 = 1;
						BC7Neg = ST3;
						if (Rstrain >= C2Neg) {//情况一
							Tstress = ZCu39stress + (Rstrain - ZCu39strain) * k382;
							Ttangent = k382;
							TCu37 = Ts1 + (Rstrain - ST1) * k2;
							if (Tstress <= (TCu37 + 0.1)) {
								CaNeg = 0;
								loadNeg = 0;
								Cu3Neg = 0;
								Cu2Neg = 0;
								Ts2 = Tstress;
								ST2 = Rstrain;
							}
						}
						else {//情况二
							Tstress = ZCu39stress + (Rstrain - ZCu39strain) * k2;
							Ttangent = k2;
							TCu37 = Ts2 + (Rstrain - ST2) * Fk3;
							if (Tstress <= (TCu37 + 0.1)) {
								CaNeg = 0;
								loadNeg = 0;
								Cu3Neg = 0;
								Cu2Neg = 0;
								Ts2 = Tstress;
								ST2 = Rstrain;
							}
						}
					}
				}
			}
		}
		else if ((Rstrain >= Cu4Neg) && (XuNeg == 0) && (segment == 4)) {//当第四段开始卸载时
			if ((Rstrain <= Xu4Neg) && (XuNeg41 == 0)) {//卸载段第一段
				CaNeg = 1;
				loadNeg = 1;
				Tstress = Ts4 + (Rstrain - ST4) * lk5;
				Ttangent = lk5;
				Cu4stress = Tstress;
				Cu4strain = Rstrain;
				ZCu4 = Rstrain;
				//拟合数据的时候，需要修改的地方
				//BC6Neg = -(C1 + (-ST4 - C1) / (C3 - C1) * (C6 - C1));
				Cbeta = -(Ts4 - Fc5) / k5;
				BC6Neg = (ST4 + Cbeta + (C3 - beta - C6));
				//BC6Neg = (Fc6 - Fc5) / k6 - C3 - Cbeta;
				//BC6Neg = -(C6 / 4.094) * (-1.91 + 3.59 * log(-ST4));
				//lk6 = (k6 / 21.19) * (47.42 - 10.581 * (-ST4) + 0.998 * pow(ST4, 2));
				//BC6Neg = -Track_L * (0.8871 * (-ST4) - 1.332);
				//lk6 = Track_K * (3.486 * pow(-ST4, 2) - 50.82 * (-ST4) + 257.4);
				/*BC6Neg = -C6;*/
				//
				TCu3 = Ts3 + (Rstrain - ST3) * k4;
				Cu3Neg = -10000000;       //当静力控制时，在加载段第四段卸载时，卸载第一段正常运行
				if (Tstress <= TCu3) {
					CaNeg = 0;
					loadNeg = 0;
					Cu3Neg = 0;
					Cu4Neg = 0;
					Ts3 = Tstress;
					ST3 = Rstrain;
				}
				if (Rstrain >= BC6Neg) {
					BC7Neg = -(C1 + (-ST4 - C1) / (C3 - C1) * (C7 - C1));
					Cu46stress = Tstress;
					Cu46strain = Rstrain;
					ZCu46stress = Tstress;
					ZCu46strain = Rstrain;
					ZCu4 = Rstrain;
					ZCu43Neg = Rstrain;
				}
			}
			else if (BC6Neg < Xu4Neg) {
				BC6Neg = Xu4Neg;
				Cu46stress = Tstress;
				Cu46strain = Rstrain;
				ZCu46stress = Tstress;
				ZCu46strain = Rstrain;
				ZCu4 = Rstrain;
				ZCu43Neg = Rstrain;
				//拟合数据的时候，需要修改的地方
				//BC7Neg = -(C1 + (-ST4 - C1) / (C3 - C1) * (C7 - C1));
				BC7Neg = (Fc7 - Fc6) / k7 + BC6Neg;
			}
			else if ((Rstrain > Xu4Neg) && (Rstrain <= BC6Neg)) {
				if (Rstrain >= ZCu4) {//卸载段第二段
					Tstress = Cu4stress + (Rstrain - Cu4strain) * lk6;
					Ttangent = lk6;
					Cu46stress = Tstress;
					Cu46strain = Rstrain;
					ZCu46stress = Tstress;
					ZCu46strain = Rstrain;
					ZCu4 = Rstrain;
					ZCu43Neg = Rstrain;
					//拟合数据的时候，需要修改的地方
					//BC7Neg = -(C1 + (-ST4 - C1) / (C3 - C1) * (C7 - C1));
					BC7Neg = (Fc7 - Fc6) / k7 + BC6Neg;
					//BC7Neg = -(C7 / 1.89) * (0.76 + 0.5 * log(-ST4));
					//lk7 = (k7 / 7.6) * (46.283 - 13.404 * (-ST4) + 1.23 * pow(ST4, 2));
					//BC7Neg = -Track_L * (qremain + 1.5);
					//lk7 = Track_K * (2.289 * pow(-ST4, 2) - 43.34 * (-ST4) + 239.9);
					/*BC7Neg = -C7;*/
					//loadNeg = 1;
					if (i != j) {  //判断圈数
						j = j + 1;
					}
				}
				else {//卸载第二段的位置再加载
					Tstress = ZCu46stress + (Rstrain - ZCu46strain) * lk5;
					Ttangent = lk5;
					Xu4Neg = 10 * C3Neg;
					TCu46 = Ts2 + (Rstrain - ST2) * Fk3;
					TCu462 = Ts3 + (Rstrain - ST3) * k4;
					if (Tstress <= (TCu46 + 0.1) || (Tstress <= (TCu462 + 0.1))) {
						if (Tstress <= (TCu46 + 0.1) && (Rstrain >= C3Neg)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu3Neg = 0;
							Ts2 = Tstress;
							ST2 = Rstrain;
						}
						if ((Tstress <= (TCu462 + 0.1)) && (Rstrain <= C3Neg)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu4Neg = 0;
							Ts3 = Tstress;
							ST3 = Rstrain;
						}
					}
				}
			}
			else if ((Rstrain >= BC6Neg) && (Rstrain <= BC7Neg)) {//卸载段第三段
				if (Rstrain >= ZCu43Neg) {
					Tstress = Cu46stress + (Rstrain - Cu46strain) * lk7;
					Ttangent = lk7;
					Cu47stress = Tstress;
					Cu47strain = Rstrain;
					ZCu47stress = Tstress;
					ZCu47strain = Rstrain;
					ZCu4 = Rstrain;
					ZCu43Neg = Rstrain;
					ZCu44 = Rstrain;
					k8 = Cu47stress / (Cu47strain + qremain);
					//Cu3 = -100000;    //
				}
				else {//在卸载第三段的位置再加载
					BC6Neg = Cu4Neg;
					Tstress = ZCu47stress + (Rstrain - ZCu47strain) * Fk1;
					Ttangent = Fk1;
					XuNeg41 = 1;
					TCu47 = Ts2 + (Rstrain - ST2) * Fk3;    //情况一
					TCu472 = Ts1 + (Rstrain - ST1) * k2;   //情况二
					TCu473 = Ts3 + (Rstrain - ST3) * k4;   //情况三
					if ((Tstress <= (TCu47 + 0.1)) || (Tstress <= (TCu472 + 0.1)) || (Tstress <= (TCu473 + 0.1))) {    //当力大于，当前位移下按照第二段或者第三段计算的力
						if (Tstress <= (TCu47 + 0.1)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu3Neg = 0;
							Ts2 = Tstress;   //于第三段相连接
							ST2 = Rstrain;   //于第三段相连接
						}
						if (Tstress <= (TCu472 + 0.1)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu2Neg = 0;
							Ts1 = Tstress;   //于第三段相连接
							ST1 = Rstrain;   //于第三段相连接
						}
						if (Tstress <= (TCu473 + 0.1) && (Rstrain <= C3Neg)) {
							CaNeg = 0;
							loadNeg = 0;
							Cu4Neg = 0;
							Ts3 = Tstress;   //于第三段相连接
							ST3 = Rstrain;   //于第三段相连接
						}
					}
				}
			}
			//else if (BC7Neg < BC6Neg) {
			//	BC7Neg = BC6Neg;
			//	Cu47stress = Tstress;
			//	Cu47strain = Rstrain;
			//	ZCu47stress = Tstress;
			//	ZCu47strain = Rstrain;
			//	ZCu4 = Rstrain;
			//	ZCu43Neg = Rstrain;
			//	ZCu44 = Rstrain;
			//	k8 = Cu47stress / (Cu47strain + qremain);
			//}
			else if (Rstrain > BC7Neg) {//卸载第四段
				if (Rstrain >= ZCu44) {
					Tstress = Cu47stress + (Rstrain - Cu47strain) * k8;
					Ttangent = k8;
					ZCu48stress = Tstress;
					ZCu48strain = Rstrain;
					ZCu44 = Rstrain;
					ZCu49stress = Tstress;
					ZCu49strain = Rstrain;
					k482 = (STs2 - ZCu49stress) / (SST2 - ZCu49strain);
				}
				else {//在卸载第四段的位置再加载
					if (Tstress >= STs1) {//再加载第一段
						Tstress = ZCu48stress + (Rstrain - ZCu48strain) * Fk1;
						Ttangent = Fk1;
						ZCu49stress = Tstress;
						ZCu49strain = Rstrain;
						k482 = (STs2 - ZCu49stress) / (SST2 - ZCu49strain);
						BC6Neg = 0;
						Xu4Neg = 0;
						XuNeg41 = 1;
						BC7Neg = ST3;
					}
					else {//再加载第二段
						BC6Neg = 0;
						Xu4Neg = 0;
						XuNeg41 = 1;
						BC7Neg = ST3;
						if (Rstrain >= C2Neg) {//情况一
							Tstress = ZCu49stress + (Rstrain - ZCu49strain) * k482;
							Ttangent = k482;
							TCu47 = Ts1 + (Rstrain - ST1) * k2;
							if (Tstress <= (TCu47 + 0.1)) {
								CaNeg = 0;
								loadNeg = 0;
								Cu3Neg = 0;
								Cu2Neg = 0;
								Cu4Neg = 0;
								Ts2 = Tstress;
								ST2 = Rstrain;
							}
						}
						else {//情况二
							Tstress = ZCu49stress + (Rstrain - ZCu49strain) * k2;
							Ttangent = k2;
							TCu47 = Ts2 + (Rstrain - ST2) * Fk3;
							if (Tstress <= (TCu47 + 0.1)) {
								CaNeg = 0;
								loadNeg = 0;
								Cu3Neg = 0;
								Cu2Neg = 0;
								Cu4Neg = 0;
								Ts2 = Tstress;
								ST2 = Rstrain;
							}
						}
					}
				}
			}
		}
		if ((Tstrain >= -0.000001) && (mode == 1)) {
			CaNeg = 0;
			XuNeg = 0;
			loadNeg = 0;
		}
		if (Tstress >= 0) {
			if (mode == 1) {
				Tstress = 0;
			}
			else if (mode == 2) {
				Res_deformation_Neg = Tstress;
			}
			Ca = 0;
			load = 0;
			Xu = 1;
			Trange = 1;
		}
	}

	return 0;

}

double
ModSMA::getStress(void)
{
	return Tstress;
}

double
ModSMA::getTangent(void)
{
	return Ttangent;
}

double
ModSMA::getStrain(void)
{
	return Tstrain;
}

int
ModSMA::commitState(void)
{
	Ca = 0;
	CaNeg = 0;
	remain = 0;
	qremain = 0;
	Cstrain = Tstrain;
	Cstress = Tstress;
	Ctangent = Ttangent;
	return 0;
}

int
ModSMA::revertToLastCommit(void)
{
	Tstrain = Cstrain;
	Tstress = Cstress;
	Ttangent = Ctangent;
	return 0;
}

int
ModSMA::revertToStart(void)
{
	Ca = 0;
	load = 0;
	loadNeg = 0;
	Xu = 0;
	//XuNeg = 0;
	CaNeg = 0;
	C1Pos = C1;
	C2Pos = C2;
	C3Pos = C3;
	C6Pos = C6;
	C7Pos = C7;
	C1Neg = -C1;
	C2Neg = -C2;
	C3Neg = -C3;
	C6Neg = -C6;
	C7Neg = -C7;
	Tstress = 0;
	Ttangent = k1;
	Rstrain = 0;
	Tstrain = 0;
	Cu2 = 0;
	Cu2Neg = 0;
	Xu2 = (SST2 - C1Pos) * 0.5 + C1Pos;
	remain = 0;
	qremain = 0;
	return 0;
}

UniaxialMaterial*
ModSMA::getCopy(void)
{
	ModSMA* theCopy =
		new ModSMA(this->getTag(), k1, k2, k3, k4, k5, k6, k7, C1, C2, C3, C6, C7, beta);



	// Copy trial state variables
	theCopy->Tstrain = Tstrain;
	theCopy->Tstress = Tstress;
	theCopy->Ttangent = Ttangent;

	theCopy->Cstrain = Cstrain;
	theCopy->Cstress = Cstress;
	theCopy->Ctangent = Ctangent;



	return theCopy;
}

int
ModSMA::sendSelf(int cTag, Channel& theChannel)
{
	int res = 0;

	static Vector data(105);

	data(0) = this->getTag();
	data(1) = k1;
	data(2) = k2;
	data(3) = k3;
	data(4) = k4;
	data(5) = k5;
	data(6) = k6;
	data(7) = k7;
	data(8) = k8;
	data(9) = k382;
	data(10) = k482;
	data(11) = C1;
	data(12) = C2;
	data(13) = C3;
	data(14) = C6;
	data(15) = C7;
	data(16) = beta;
	data(17) = remain;
	data(18) = Rstrain;
	data(19) = C1Pos;
	data(20) = C2Pos;
	data(21) = C3Pos;
	data(22) = C6Pos;
	data(23) = BC6;
	data(24) = C7Pos;
	data(25) = BC7;
	data(26) = Cu2;
	data(27) = Cu3;
	data(28) = Cu4;
	data(29) = STs2;
	data(30) = SST2;
	data(31) = Ts1;
	data(32) = Ts2;
	data(33) = Ts3;
	data(34) = Ts4;
	data(35) = ST1;
	data(36) = ST2;
	data(37) = ST3;
	data(38) = ST4;
	data(39) = Xu2;
	data(40) = Xu3;
	data(41) = Xu31;
	data(42) = XuNeg31;
	data(43) = Xu4;
	data(44) = Xu41;
	data(45) = XuNeg41;
	data(46) = Cu2strain;
	data(47) = ZCu2strain;
	data(48) = Cu2stress;
	data(49) = ZCu2stress;
	data(50) = Cu3strain;
	data(51) = ZCu3strain;
	data(52) = Cu3stress;
	data(53) = ZCu3stress;
	data(54) = Cu36strain;
	data(55) = ZCu36strain;
	data(56) = Cu36stress;
	data(57) = ZCu36stress;
	data(58) = Cu37strain;
	data(59) = ZCu37strain;
	data(60) = Cu37stress;
	data(61) = ZCu37stress;
	data(62) = Cu38strain;
	data(63) = ZCu38strain;
	data(64) = Cu38stress;
	data(65) = ZCu38stress;
	data(66) = ZCu39stress;
	data(67) = ZCu39strain;
	data(68) = Cu4strain;
	data(69) = ZCu4strain;
	data(70) = Cu4stress;
	data(71) = ZCu4stress;
	data(72) = Cu46strain;
	data(73) = ZCu46strain;
	data(74) = Cu46stress;
	data(75) = ZCu46stress;
	data(76) = Cu47strain;
	data(77) = ZCu47strain;
	data(78) = Cu47stress;
	data(79) = ZCu47stress;
	data(80) = Cu48strain;
	data(81) = ZCu48strain;
	data(82) = Cu48stress;
	data(83) = ZCu48stress;
	data(84) = ZCu49stress;
	data(85) = ZCu49strain;
	data(86) = Ca;
	data(87) = CaNeg;
	data(88) = Xu;
	data(89) = XuNeg;
	data(90) = ZCu2;
	data(91) = TCu2;
	data(92) = ZCu3;
	data(93) = TCu3;
	data(94) = TCu36;
	data(95) = TCu37;
	data(96) = TCu38;
	data(97) = ZCu4;
	data(98) = ZCu43;
	data(99) = ZCu44;
	data(100) = TCu4;
	data(101) = TCu46;
	data(102) = TCu47;
	data(103) = TCu48;
	data(104) = ZCu5;
	data(105) = segment;


	res = theChannel.sendVector(this->getDbTag(), cTag, data);
	if (res < 0)
		opserr << "ModSMA::sendSelf() - failed to send data\n";

	return res;
}

int
ModSMA::recvSelf(int cTag, Channel& theChannel,
	FEM_ObjectBroker& theBroker)
{
	int res = 0;

	static Vector data(105);
	res = theChannel.recvVector(this->getDbTag(), cTag, data);

	if (res < 0) {
		opserr << "ModSMA::recvSelf() - failed to receive data\n";
		this->setTag(0);
	}
	else {
		this->setTag((int)data(0));
		k1 = data(1);
		k2 = data(2);
		k3 = data(3);
		k4 = data(4);
		k5 = data(5);
		k6 = data(6);
		k7 = data(7);
		k8 = data(8);
		k382 = data(9);
		k482 = data(10);
		C1 = data(11);
		C2 = data(12);
		C3 = data(13);
		C6 = data(14);
		C7 = data(15);
		beta = data(16);
		remain = data(17);
		Rstrain = data(18);
		C1Pos = data(19);
		C2Pos = data(20);
		C3Pos = data(21);
		C6Pos = data(22);
		BC6 = data(23);
		C7Pos = data(24);
		BC7 = data(25);
		Cu2 = data(26);
		Cu3 = data(27);
		Cu4 = data(28);
		STs2 = data(29);
		SST2 = data(30);
		Ts1 = data(31);
		Ts2 = data(32);
		Ts3 = data(33);
		Ts4 = data(34);
		ST1 = data(35);
		ST2 = data(36);
		ST3 = data(37);
		ST4 = data(38);
		Xu2 = data(39);
		Xu3 = data(40);
		Xu31 = data(41);
		XuNeg31 = data(42);
		Xu4 = data(43);
		Xu41 = data(44);
		XuNeg41 = data(45);
		Cu2strain = data(46);
		ZCu2strain = data(47);
		Cu2stress = data(48);
		ZCu2stress = data(49);
		Cu3strain = data(50);
		ZCu3strain = data(51);
		Cu3stress = data(52);
		ZCu3stress = data(53);
		Cu36strain = data(54);
		ZCu36strain = data(55);
		Cu36stress = data(56);
		ZCu36stress = data(57);
		Cu37strain = data(58);
		ZCu37strain = data(59);
		Cu37stress = data(60);
		ZCu37stress = data(61);
		Cu38strain = data(62);
		ZCu38strain = data(63);
		Cu38stress = data(64);
		ZCu38stress = data(65);
		ZCu39stress = data(66);
		ZCu39strain = data(67);
		Cu4strain = data(68);
		ZCu4strain = data(69);
		Cu4stress = data(70);
		ZCu4stress = data(71);
		Cu46strain = data(72);
		ZCu46strain = data(73);
		Cu46stress = data(74);
		ZCu46stress = data(75);
		Cu47strain = data(76);
		ZCu47strain = data(77);
		Cu47stress = data(78);
		ZCu47stress = data(79);
		Cu48strain = data(80);
		ZCu48strain = data(81);
		Cu48stress = data(82);
		ZCu48stress = data(83);
		ZCu49stress = data(84);
		ZCu49strain = data(85);
		Ca = data(86);
		CaNeg = data(87);
		Xu = data(88);
		XuNeg = data(89);
		ZCu2 = data(90);
		TCu2 = data(91);
		ZCu3 = data(92);
		TCu3 = data(93);
		TCu36 = data(94);
		TCu37 = data(95);
		TCu38 = data(96);
		ZCu4 = data(97);
		ZCu43 = data(98);
		ZCu44 = data(99);
		TCu4 = data(100);
		TCu46 = data(101);
		TCu47 = data(102);
		TCu48 = data(103);
		ZCu5 = data(104);
		segment = data(105);

	}

	return res;
}

void
ModSMA::Print(OPS_Stream& s, int flag)
{
	s << "ModSMA, tag: " << this->getTag() << endln;
	s << "  k1: " << k1 << endln;
	s << "  k2: " << k2 << endln;
	s << "  k3: " << k3 << endln;
	s << "  k4: " << k4 << endln;
	s << "  k5: " << k5 << endln;
	s << "  k6: " << k6 << endln;
	s << "  k7: " << k7 << endln;
}

