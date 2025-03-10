/*
 *   FAC - Flexible Atomic Code
 *   Copyright (C) 2001-2015 Ming Feng Gu
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _RADIAL_H_
#define _RADIAL_H_

#include <time.h>
#include <math.h>

#include "global.h"
#include "nucleus.h"
#include "interpolation.h"
#include "grid.h"
#include "coulomb.h"
#include "orbital.h"
#include "config.h"
#include "angular.h"
#include "recouple.h"

#ifdef PERFORM_STATISTICS
typedef struct _RAD_TIMING_ {
  double radial_1e;
  double radial_2e;
  double dirac;
  double radial_slater;
} RAD_TIMING;

int GetRadTiming(RAD_TIMING *t);
#endif

typedef struct _orbmap_ {
  ORBITAL **opn;
  ORBITAL **onn;
  int nmax;
  MULTI *ozn;
  int ifm;
  double cfm, xfm;
} ORBMAP;

double *WLarge(ORBITAL *orb);
double *WSmall(ORBITAL *orb);
int GetBoundary(double *rb, double *b, int *nmax, double *dr);
int SetBoundaryMaster(int nmax, double p, double bqp, double rf);
int SetBoundary(int nmax, double p, double bqp, double rf,
		int nr, int ng, int *kg, char *s, int n0, int n1,
		int n0d, int n1d, int k0, int k1);
void PrintQED();
int RadialOverlaps(char *fn, int kappa);
void SetSlaterCut(int k0, int k1);
void SetPotentialMode(int m, double h, double ihx,
		      double dhx, double hx0, double hx1);
void SetSE(int n, int m, int s, int p);
void SetModSE(double ose0, double ose1, double ase,
	      double cse0, double cse1, double ise);
void SetVP(int n);
void SetBreit(int n, int m, int n0, double x0, int n1);
int GetBreit(void);
void SetMS(int nms, int sms);
int SetAWGrid(int n, double min, double max);
int GetAWGrid(double **a);
int SetRadialGrid(int maxrp, double ratio, double asymp,
		  double rmin, double qr);
void SetPotential(AVERAGE_CONFIG *acfg, int iter);
int PotentialHX(AVERAGE_CONFIG *acfg, double *u, int iter);
int PotentialHX1(AVERAGE_CONFIG *acfg, int iter, int md);
void FixAsymptoticPot(int jmax, int md, int iter, double *w,
		      double *u, double *ue1, double *ue);
void SetReferencePotential(POTENTIAL *h, POTENTIAL *p, int hlike);
POTENTIAL *RadialPotential(void);
int GetPotential(char *s, int m);
void OrbitalStats(char *s, int nmax, int kmax);
void CopyPotentialOMP(int i);
double GetResidualZ(void);
double GetRMax(void);
void SetScreenConfig(int iter);
/* solve the dirac equation for the given orbital */
ORBITAL *SolveAltOrbital(ORBITAL *orb, POTENTIAL *p);
void Orthogonalize(ORBITAL *orb);
int SolveDirac(ORBITAL *orb);
int WaveFuncTableOrb(char *s, ORBITAL *orb);
int WaveFuncTable(char *s, int n, int kappa, double e);

/* get the index of the given orbital in the table */
//int OrbitalIndexNoLock(int n, int kappa, double energy);
int OrbitalIndex(int n, int kappa, double energy);
int OrbitalExistsNoLock(int n, int kappa, double energy);
int OrbitalExists(int n, int kappa, double energy);
//int AddOrbital(ORBITAL *orb);
ORBITAL *GetOrbital(int k);
ORBITAL *GetOrbitalSolved(int k);
ORBITAL *GetOrbitalSolvedNoLock(int k);
void SetOrbMap(int k, int n0, int n1);
void SetEnergyIndex(int idx[3], int kappa, double energy);
void AddOrbMap(ORBITAL *orb);
void RemoveOrbMap(int m);
ORBITAL *GetNewOrbitalNoLock(int n, int kappa, double e, ORBITAL **solve);
ORBITAL *GetNewOrbital(int n, int kappa, double e);
int GetNumBounds(void);
int GetNumOrbitals(void);
int GetNumContinua(void);

double CoulombEnergyShell(CONFIG *cfg, int i);
void ShiftOrbitalEnergy(CONFIG *cfg);
double GetPhaseShift(int k);

/* radial optimization */
int SetAverageConfig(int nshells, int *n, int *kappa, double *nq);
void SetConfigEnergyMode(int m);
int ConfigEnergyMode(void);
int OptimizeMaxIter(void);
void SetOptimizeMaxIter(int m);
void SetOptimizeStabilizer(double m);
void SetOptimizeTolerance(double c);
void SetOptimizePrint(int m);
void SetOptimizeControl(double tolerence, double stablizer, 
			int maxiter, int iprint);
void SetScreening(int n_screen, int *screened_n, 
		  double screened_harge, int kl);
int OptimizeRadial(int ng, int *kg, int ic, double *weight, int ife);
int OptimizeRadialWSC(int ng, int *kg, int ic, double *weight, int ife);
int RefineRadial(int m, int n, int maxfun, int msglvl);
double ConfigEnergyShiftCI(int nrs0, int nrs1);
double ConfigEnergyShift(int ns, SHELL *bra, int ia, int ib, int m2);
double ConfigEnergyVariance(int ns, SHELL *bra, int ia, int ib, int m2);
int ConfigEnergy(int m, int mr, int ng, int *kg);
double TotalEnergyGroup(int kg);
double TotalEnergyGroups(int ng, int *kg, double *w);
double TotalEnergyGroupMode(int kg, int md);
double ZerothEnergyConfig(CONFIG *cfg);
double ZerothResidualConfig(CONFIG *cfg);
double ConfigHamilton(CONFIG *cfg, ORBITAL *orb0, ORBITAL *orb1, double xdf);
double AverageEnergyConfig(CONFIG *cfg);
double AverageEnergyConfigMode(CONFIG *cfg, int md);
double AverageEnergyAvgConfig(AVERAGE_CONFIG *cfg);
void DiExAvgConfig(AVERAGE_CONFIG *cfg, double *d0, double *d1);
void DiExConfig(CONFIG *cfg, double *d0, double *d1);

/* routines for radial integral calculations */
int GetYk(int k, double *yk, ORBITAL *orb1, ORBITAL *orb2, 
	  int k1, int k2, int type);
int Integrate(double *f, ORBITAL *orb1, ORBITAL *orb2, int type, double *r, int id);
int IntegrateSubRegion(int i0, int i1, 
		       double *f, ORBITAL *orb1, ORBITAL *orb2,
		       int t, double *r, int m);
int IntegrateSinCos(int j, double *x, double *y, 
		    double *phase, double *dphase, 
		    int i0, double *r, int t);
int IntegrateSinCos1(int j, double *x, double scx, double *y, double scy,
		     double *phase, double *dphase, 
		     int i0, double *r, int t);
int IntegrateSinCos2(int j, double *x, double scx, double *y, double scy,
		     double *phase, double *dphase, 
		     int i0, double *r, int t);
int SlaterTotal(double *sd, double *se, int *js, int *ks, int k, int mode);
double *Vinti(int k0, int k1);
double QED1E(int k0, int k1);
double SelfEnergyExotic(ORBITAL *orb);
double SelfEnergy(ORBITAL *orb1, ORBITAL *orb2);
double SelfEnergyRatioWelton(ORBITAL *orb, ORBITAL *horb);
double SelfEnergyRatio(ORBITAL *orb, ORBITAL *horb);
int Slater(double *s, int k0, int k1, int k2, int k3, int k, int mode);
int BreitX(ORBITAL *orb0, ORBITAL *orb1, int k, int m, int w, int mbr,
	   double e, double *y);
double BreitC(int n, int m, int k, int k0, int k1, int k2, int k3);
double BreitS(int k0, int k1, int k2, int k3, int k);
int BreitSYK(int k0, int k1, int k, double *z);
double BreitI(int n, int k0, int k1, int k2, int k3, int m);
double Breit(int k0, int k1, int k2, int k3, int k,
	     int kp0, int kp1, int kp2, int kp3,
	     int kl0, int kl1, int kl2, int kl3, int mbr);
void SortSlaterKey(int *kd);
void PrepSlater(int ib0, int iu0, int ib1, int iu1,
		int ib2, int iu2, int ib3, int iu3);
void PrepYK(int n, int m);
double ZerothHamilton(ORBITAL *orb0, ORBITAL *orb1);
int ResidualPotential(double *s, int k0, int k1);
double MeanPotential(int k0, int k1);
int FreeResidualArray(void);
int FreeMultipoleArray(void);
int FreeSlaterArray(void);
int FreeSimpleArray(MULTI *ma);
int FreeMomentsArray(void);
int FreeGOSArray(void);
void FreezeOrbital(char *s, int m);
double RadialMoments(int m, int k1, int k2);
double MultipoleRadialNR(int m, int k1, int k2, int guage);
int MultipoleRadialFRGrid(double **p, int m, int k1, int k2, int guage);
double MultipoleRadialFR(double aw, int m, int k1, int k2, int guage);
double InterpolateMultipole(double aw2, int n, double *x, double *y);
double *GeneralizedMoments(int k0, int k1, int m);
void PrintGeneralizedMoments(char *fn, int m, int n0, int k0, int n1, int k1, 
			     double e1);
int ClearOrbitalTable(int m);
void LimitArrayRadial(int m, double n);
int InitRadial(void);
int ReinitRadial(int m);
void SetRadialCleanFlags(void);
int TestIntegrate(void);
double IntRadJn(int n0, int k0, double e0,
		int n1, int k1, double e1,
		int n, int m, char *fn);
int RestorePotential(char *fn, POTENTIAL *p);
int SavePotential(char *fn, POTENTIAL *p);
int ModifyPotential(char *fn, POTENTIAL *p);
void OptimizeModSE(int n, int ka, double dr, int ni);
void RemoveOrbitalLock(void);
double GetHXS(POTENTIAL *p);
int AddNewConfigToList(int k, int ni, int *kc, CONFIG *c0,
		       int nb, int **kbc,
		       int nc, SHELL_RESTRICTION *sr, int checknew, int mar);
int ConfigSD(int m, int ng, int *kg, char *s, char *gn1, char *gn2,
	     int n0, int n1, int n0d, int n1d, int k0, int k1,
	     int ngb, int *kgb, double sth);
void SolvePseudo(int kmin, int kmax, int nb, int nmax, int nd, double xdf);
void SolveDFKappa(int ka, int nmax, double xdf);
double *WorkSpace();
void AllocDWS(int n);
void AllocWorkSpace(int n);
void AllocPotMem(POTENTIAL *p, int n);
void SetPotDP(POTENTIAL *p);
void ExpectationValue(char *ifn, char *ofn, int n, int *ilev, double a, int t);
void ElectronDensity(char *ofn, int n, int *ilev, int t);
void SetOptionRadial(char *s, char *sp, int ip, double dp);
void SaveRadialMultipole(char *fn, int n, int nk, int *ks, int g);
void LoadRadialMultipole(char *fn);
void PlasmaScreen(int m, int vxf,
		  double zps, double nps, double tps, double ups);
void SetOrbNMax(int kmin, int kmax, int nmax);
int GetOrbNMax(int kappa, int j);
int SetNMaxKappa(int kappa, int nmax);
int DensityAsymptote(double *d, double *a, double *b);
double NBoundAA(int ns, int *n, int *ka, double *nq, double *et, double *nqc,
		double u, double t, double e0, double *nqf);
void AverageAtom(char *pref, int m, double d, double t, double ztol);
void SetPotentialN(void);
int SetScreenDensity(AVERAGE_CONFIG *acfg, int iter, int md);
void LoadSCPot(char *fn);
void SaveSCPot(int md, char *fn, double sca);
double FreeKineticEnergy(double ne, double t, double *u);
double EffectiveTe(double ne, double ek);
void SetSlaterScale(int m, char *s0, char *s1, double x);
AVERAGE_CONFIG *AverageConfig(void);

#endif

