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

#include "radial.h"
#include "mpiutil.h"
#include "init.h"
#include "cf77.h"
#include "structure.h"
#include <errno.h>

static char *rcsid="$Id$";
#if __GNUC__ == 2
#define USE(var) static void * use_##var = (&use_##var, (void *) &var) 
USE (rcsid);
#endif

typedef struct _FLTARY_ {
  short npts;
  float *yk;
} FLTARY;

#define NHXN 30
#define NXS 15
#define NXS2 (2*NXS+1)
static double _hxn[NHXN] = {2,  3,  4,  5,  6,  7,  8,  9, 10,
			    11, 12, 13, 14, 15, 16, 17, 18,
			    19, 20, 21, 22, 23, 24, 25, 26, 27,
			    28, 29, 30, 31};
static double _hx0[2][NHXN] = {
  {0.49464443,  0.4944842 ,  0.47671812,  0.47368338,  0.46881982,
   0.4650677 ,  0.46239243,  0.45992017,  0.4581007 ,  0.45699729,
   0.45327464,  0.45147903,  0.45018784,  0.44901484,  0.44783271,
   0.44677685,  0.44601044,  0.44668505,  0.44690311,  0.44747163,
   0.44809634,  0.44896082,  0.45032069,  0.4514605 ,  0.45283192,
   0.45420841,  0.45580976,  0.45472978,  0.45249835, 0.45},
  {0.60392457,  0.5963838 ,  0.54396742,  0.53136923,  0.53471311,
   0.53206107,  0.52694942,  0.52089717,  0.51642186,  0.53134076,
   0.48894837,  0.47899983,  0.47580524,  0.47583937,  0.4763066 ,
   0.47516619,  0.47327929,  0.47544085,  0.47804916,  0.48123176,
   0.4848968 ,  0.4886115 ,  0.49402954,  0.49848388,  0.50336365,
   0.50842387,  0.51389326,  0.53564154,  0.55618328, 0.55}};
static double _hx1[2][NHXN] = {
  {-2.24999797e-05,  -2.09402638e-05,  -1.97068001e-05,
   -1.86529759e-05,  -1.73178539e-05,  -1.95629593e-05,
   -2.21086675e-05,  -2.11895870e-05,  -2.01426809e-05,
   -1.98210143e-05,  -1.92998222e-05,  -1.86920564e-05,
   -1.82455335e-05,  -1.84696909e-05,  -1.87351162e-05,
   -1.80283589e-05,  -1.73407781e-05,  -1.69192139e-05,
   -1.63363906e-05,  -1.58294488e-05,  -1.53107601e-05,
   -1.53014957e-05,  -1.54457903e-05,  -1.50586829e-05,
   -1.47144622e-05,  -1.43879181e-05,  -1.40649716e-05,
   -1.40581261e-05,  -1.38378555e-05, -1.4e-5},
  {-4.83974367e-05,  -2.69133100e-05,  -2.41090948e-05,
   -2.21989673e-05,  -1.94076264e-05,  -1.97061814e-05,
   -2.07157339e-05,  -1.88678066e-05,  -1.77080751e-05,
   -2.23815832e-05,  -1.76272492e-05,  -1.70901358e-05,
   -1.70196986e-05,  -1.72463938e-05,  -1.72980241e-05,
   -1.61671348e-05,  -1.50179117e-05,  -1.48463176e-05,
   -1.47797417e-05,  -1.47793769e-05,  -1.48393532e-05,
   -1.53064076e-05,  -1.62570909e-05,  -1.63962294e-05,
   -1.65777367e-05,  -1.67306107e-05,  -1.69330862e-05,
   -1.95968009e-05,  -2.12996663e-05, -2.0e-5}};

static POTENTIAL *potential = NULL;
static POTENTIAL *hpotential = NULL;
static POTENTIAL *rpotential = NULL;

#define Large(orb) ((orb)->wfun)
#define Small(orb) ((orb)->wfun + potential->maxrp)

static ARRAY *orbitals;
static int n_orbitals;
static int n_continua;
static int _orbitals_block = ORBITALS_BLOCK;

static int _nws = 0;
static double *_dws = NULL;
static double *_dwork;
static double *_dwork1;
static double *_dwork2;
static double *_dwork3;
static double *_dwork4;
static double *_dwork5;
static double *_dwork6;
static double *_dwork7;
static double *_dwork8;
static double *_dwork9;
static double *_dwork10;
static double *_dwork11;
static double *_dwork12;
static double *_dwork13;
static double *_dwork14;
static double *_dwork15;
static double *_dwork16;
static double *_dwork17;
static double *_phase;
static double *_dphase;
static double *_dphasep;
static double *_yk;
static double *_zk;
static double *_xk;

#pragma omp threadprivate(potential,hpotential,rpotential,_nws,_dws,_dwork,_dwork1,_dwork2,_dwork3,_dwork4,_dwork5,_dwork6,_dwork7,_dwork8,_dwork9,_dwork10,_dwork11,_dwork12,_dwork13,_dwork14,_dwork15,_dwork16,_dwork17,_phase,_dphase,_dphasep,_yk,_zk,_xk)

static int _jsi = 0;
static double _wsi = 0.995;
static double _csi = 0.55;
static double _xhx = 0.1;
static int _mhx = 0;
static double _ihxmin = MINIHX;
static double **_refine_wfb = NULL;
static double _refine_xtol = EPS5;
static double _refine_scale = 0.1;
static int _refine_msglvl = 0;
static int _refine_np = 6;
static int _refine_maxfun = 10000;
static int _refine_mode = 1;
static int _refine_pj = -1;
static int _refine_em = 0;
static int _print_spm = 0;
static int _znbaa = 0;
static double _psemax = 7.5;
static int _psnfmin = 1;
static int _psnfmax = 6;
static int _psnmax = 0;
static int _psnmin = 0;
static int _pskmax = 25;
static int _slater_kmax = -1;
static int _orbnmax_print = 0;
static double _free_threshold = 0.0;
static double _gridqr = GRIDQR;
static double _gridasymp = GRIDASYMP;
static double _gridratio = GRIDRATIO;
static double _gridrmin = GRIDRMIN;
static double _gridrmax = 0.0;
static double _gridemax = 0.0;
static int _maxrp = DMAXRP;
static int _intscm = 0;
static double _intscp1 = 0.025;
static double _intscp2 = 0.5;
static double _intscp3 = 0.025;

static struct {
  int nr, md, jmax;
  double *rg;
  double *xg;
  double *dg;
  double *eg;
} _scpot = {0, -1, 0, NULL, NULL, NULL};

#define NDPH 6
static struct {
  double stabilizer;
  double tolerance; /* tolerance for self-consistency */
  int maxiter; /* max iter. for self-consistency */
  double screened_charge; 
  int screened_kl;
  int n_screen;
  int *screened_n;
  int iprint; /* printing infomation in each iteration. */
  int iset;
  int mce;
  double dph[2][NDPH];
  double rph[2][NDPH];
  double sta[2];
} optimize_control = {OPTSTABLE, OPTTOL, OPTNITER, 
		      1.0, 1, 0, NULL, OPTPRINT, 0, 0};
static int _acfg_wmode = 2;
static struct {
  int kl0;
  int kl1;
} slater_cut = {1000000, 1000000};

static struct {
  int se;
  int mse;
  int sse;
  int pse;
  int vp;
  int nms;
  int sms;
  int br;
  int mbr;
  int nbr;
  double xbr;
  int minbr;
  double ose0, ose1, ase;
  double cse0, cse1, ise;
} qed = {QEDSE, QEDMSE, 0, 0, QEDVP, QEDNMS, QEDSMS,
	 QEDBREIT, QEDMBREIT, QEDNBREIT, 0.01, 0,
	 1.0, 1.0, 3.0,
	 0.07, 0.05, 1.5};

static int _orthogonalize_mode = 1;
static int _korbmap = KORBMAP;
static int _norbmap0 = NORBMAP0;
static int _norbmap1 = NORBMAP1;
static ORBMAP *_orbmap = NULL;

static AVERAGE_CONFIG average_config = {0, 0, 0, 0, NULL, NULL,
					NULL, NULL, 0, NULL, NULL};

static MULTI *slater_array;
static MULTI *xbreit_array[5];
static MULTI *wbreit_array;
static MULTI *breit_array;
static MULTI *vinti_array;
static MULTI *qed1e_array;
static MULTI *residual_array;
static MULTI *multipole_array; 
static MULTI *moments_array;
static MULTI *gos_array;
static MULTI *yk_array;

#define MAXKSSC 16
#define MAXNSSC 64
static float _slater_scale[MAXKSSC][MAXNSSC][MAXNSSC];

#define MAXNAW 128
static int n_awgrid = 0;
static int uta_awgrid = 0;
static double awgrid[MAXNAW];
static double _awmin = 1e-4;
#define MAXMP 8
static int _nmp = 0;
static double *_dmp[2][2][MAXMP];

static int _config_energy = -1;
static int _config_energy_psr = 0;
static char _config_energy_pfn[1024];
static double _config_energy_csi = 1e10;
static double _sturm_idx = 0;
static double _sturm_eref = 0;
static int _sturm_nref = 0;
static int _sturm_kref = -1;
static int _print_maxiter = 0;
static double _aaztol = 0.0;
static int _sc_print = 0;
static int _sc_niter = 12;
static int _sc_miter = 128;
static double _sc_wmin = 0.25;
static double _hx_wmin = 0.1;
static double _maxsta = 0.75;
static double _minsta = 0.05;
static double _incsta = 1.25;
static double _rfsta = 2.5;
static double _dfsta = 0.25;
static double _minke = 1.0;
static int _sc_vxf = 1;
static char _sc_cfg[1024] = "";
static int _sc_lepton = 0;
static int _sc_maxiter = 5;
static double _sc_mass = -1.;
static double _sc_charge = -1.;
static double _sc_tol = 1e-3;
static double _sc_fmrp = 3.0;

static double PhaseRDependent(double x, double eta, double b);

#ifdef PERFORM_STATISTICS
static RAD_TIMING rad_timing = {0, 0, 0, 0};
int GetRadTiming(RAD_TIMING *t) {
  memcpy(t, &rad_timing, sizeof(RAD_TIMING));
  return 0;
}
#endif

AVERAGE_CONFIG *AverageConfig(void) {
  return &average_config;
}

void LoadSCPot(char *fn) { 
  FILE *f;
  char buf[1024];
  int i;
  double a;
  
  if (_scpot.nr > 0) {
    _scpot.nr = 0;
    _scpot.md = -1;
    _scpot.jmax = 0.0;
    free(_scpot.rg);
    free(_scpot.xg);
    free(_scpot.dg);
    free(_scpot.eg);
  }
  if (fn == NULL) return;
  
  f = fopen(fn, "r");
  if (f == NULL) {
    printf("cannot open file: %s\n", fn);
    return;
  }
  if (NULL == fgets(buf, 1024, f)) {
    fclose(f);
    return;
  }
  sscanf(buf, "# %d %d", &_scpot.nr, &_scpot.md);
  if (_scpot.nr <= 0) {
    fclose(f);
    printf("zero dim for scpot: %d %d\n", _scpot.nr, _scpot.md);
    return;
  }
  _scpot.rg = malloc(sizeof(double)*_scpot.nr);
  _scpot.dg = malloc(sizeof(double)*_scpot.nr);
  _scpot.eg = malloc(sizeof(double)*_scpot.nr);
  _scpot.xg = malloc(sizeof(double)*_scpot.nr);
  for (i = 0; i < _scpot.nr; i++) {
    _scpot.dg[i] = 0.0;
    _scpot.eg[i] = 0.0;
    _scpot.xg[i] = 0.0;
  }
  a = _RBOHR/RBOHR;
  i = 0;
  while (i < _scpot.nr) {
    if (NULL == fgets(buf, 1024, f)) break;
    if (buf[0] == '#') continue;
    sscanf(buf, "%lg %lg %lg", &_scpot.rg[i], &_scpot.dg[i], &_scpot.eg[i]);
    _scpot.rg[i] *= a;
    if (_scpot.md == 0 || _scpot.md == 2) {
      _scpot.dg[i] /= a;
      _scpot.eg[i] /= a;
    }
    i++;
  }
  fclose(f);
}

void SaveSCPot(int md, char *fn, double sca) {
  FILE *f;
  int i;
  double r, d0, d1, a;

  a = RBOHR/_RBOHR;
  if (fn == NULL) {
    if (_scpot.nr > 0) {
      _scpot.nr = 0;
      _scpot.md = -1;
      _scpot.jmax = 0.0;
      free(_scpot.rg);
      free(_scpot.xg);
      free(_scpot.dg);
      free(_scpot.eg);
    }
    if (md < 0) return;
    _scpot.nr = potential->maxrp;
    _scpot.md = 3;
    _scpot.rg = malloc(sizeof(double)*_scpot.nr);
    _scpot.dg = malloc(sizeof(double)*_scpot.nr);
    _scpot.eg = malloc(sizeof(double)*_scpot.nr);
    _scpot.xg = malloc(sizeof(double)*_scpot.nr);
    for (i = 0; i < _scpot.nr; i++) {
      _scpot.dg[i] = 0.0;
      _scpot.eg[i] = 0.0;
      _scpot.xg[i] = 0.0;
    }
    for (i = 0; i < potential->maxrp; i++) {
      r = potential->rad[i]*a;
      d0 = potential->NPS[i]/a;
      if (sca > 0) d0 *= sca;
      d1 = 0.0;
      _scpot.rg[i] = r;
      _scpot.dg[i] = d0;
      _scpot.eg[i] = d1;
    }
    return;
  }
  f = fopen(fn, "w");
  if (f == NULL) {
    printf("cannot open file: %s\n", fn);
    return;
  }
  fprintf(f, "# %d %d\n", potential->maxrp, 3);
  for (i = 0; i < potential->maxrp; i++) {
    r = potential->rad[i]*a;
    d0 = potential->NPS[i]/a;
    if (sca > 0) d0 *= sca;
    d1 = 0.0;
    fprintf(f, "%15.8E %15.8E %15.8E\n", r, d0, d1);    
  }
  fclose(f);
}

void SetOptSTA(int i, int iter) {
  int k;
  double r, r1, r2, d, ri;
  if (iter == 0) {
    optimize_control.sta[i] = 1.0;
    return;
  }
  if (iter <= NDPH) {
    optimize_control.sta[i] = optimize_control.stabilizer;
    return;
  }
  r1 = 0.0;
  r2 = 0.0;
  d = 0.0;
  for (k = 0; k < NDPH; k++) {
    ri = optimize_control.rph[i][k];
    if (ri > r2) r2 = ri;
    r1 += ri;
    d += optimize_control.dph[i][k];    
  }
  d /= NDPH;
  r1 /= NDPH;
  d /= Max(0.1,optimize_control.sta[i]);
  r = exp(-_dfsta*d)*optimize_control.stabilizer/r1;
  if (r2 > 0.75) r *= exp(-_rfsta*(r2-0.75));
  if (r > _maxsta) r = _maxsta;
  if (r < _minsta) r = _minsta;
  ri = _incsta/r2;
  ri = Max(0.75, ri);
  ri = Min(1.25, ri);
  if (d < 1E-4*optimize_control.sta[i]) ri = Max(1.01, ri);
  d = optimize_control.sta[i]*ri;
  if (r > d) r = d;
  d = _minsta/25;
  if (r < d) r = d;
  if (r > _maxsta) r = _maxsta;
  optimize_control.sta[i] = r;
}

void SetOptDPH(int i, double d, int iter) {
  int k;
  if (i < 0) {
    for (i = 0; i < 2; i++) {
      for (k = 0; k < NDPH; k++) {
	optimize_control.dph[i][k] = 0.0;
	optimize_control.rph[i][k] = 0.75;
      }
      optimize_control.sta[i] = optimize_control.stabilizer;
    }
  } else {
    for (k = 0; k < NDPH-1; k++) {
      optimize_control.dph[i][k] = optimize_control.dph[i][k+1];
      optimize_control.rph[i][k] = optimize_control.rph[i][k+1];
    }
    optimize_control.dph[i][k] = d;
    if (optimize_control.dph[i][k-1] > 0 && iter >= NDPH) {
      optimize_control.rph[i][k] = d/optimize_control.dph[i][k-1];
    } else {
      optimize_control.rph[i][k] = 0.75;
    }
    SetOptSTA(i, iter);
  }
}

void LoadRadialMultipole(char *fn) {
  int i, j, k, g, ak, ik, ig;
  char s1[16], s2[16];
  int n1, n2, naw, r, nline, nsq;
  double a, *y;
  FILE *f;
  
  if (fn == NULL || strlen(fn) == 0) {
    for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++) {
	for (k = 0; k < MAXMP; k++) {
	  if (_nmp > 0 && _dmp[i][j][k]) free(_dmp[i][j][k]);
	  _dmp[i][j][k] = NULL;
	}
      }
    }
    _nmp = 0;
    return;
  }
  f = fopen(fn, "r");
  if (f == NULL) {
    printf("cannot open file: %s\n", fn);
    return;
  }
  
  r = fscanf(f, "%s %s %d %d %d %d %d", s1, s2, &k, &g, &n1, &n2, &naw);
  if (r == EOF) {
    return;
  }
  _nmp = n1;
  int nmp2 = _nmp*_nmp;
  nsq = nmp2*nmp2*naw;
  if (nsq <= 0) {
    printf("invalid nmp: %d %d %d\n", _nmp, naw, nsq);
    return;
  }
  n_awgrid = naw;
  for (i = 0; i < naw; i++) {
    a = 0.0;
    r = fscanf(f, "%lg", &a);
    if (r != 1) {
      printf("invalid multipole awgrid\n");
      return;
    }
    awgrid[i] = a;
  }
  nline = 1;
  while (1) {
    nline++;
    r = fscanf(f, "%s %s %d %d %d %d %d", s1, s2, &k, &g, &n1, &n2, &naw);
    if (r == EOF) break;
    if (r != 7) {
      printf("invalid row: %s %s\n", s1, s2);
      break;;
    }
    if (naw != n_awgrid) {
      printf("invalid multipole naw: %d %d\n", nline, naw);
      break;
    }
    ak = abs(k);
    if (ak == 0 || ak > MAXMP) {
      printf("invalid multipole rank: %d %d\n", nline, k);
      break;
    }
    if (k < 0) ik = 0;
    else ik = 1;
    if (g == G_COULOMB) ig = 0;
    else if (g == G_BABUSHKIN) ig = 1;
    else {
      printf("invalid gauge option: %d %d\n", nline, g);
      break;
    }
    y = _dmp[ik][ig][ak-1];
    if (y == NULL) {
      y = malloc(sizeof(double)*nsq);
      _dmp[ik][ig][ak-1] = y;
      for (j = 0; j < nsq; j++) {
	y[j] = 0.0;
      }
    }
    j = naw*(n1*nmp2+n2);
    for (i = 0; i < naw; i++) {
      a = 0.0;
      r = fscanf(f, "%lg", &a);
      if (r != 1) {
	printf("invalid multipole value: %d %d\n", nline, i);
	break;
      }
      y[j+i] = a;
      //printf("%s %s %d %d %d %d %d %d %d %d %g\n", s1, s2, k, g, ik, ig, n1, n2, i, j, a);
    }
  }
  printf("LoadRadialMultipole %d lines from %s\n", nline, fn);
  fclose(f);
}
    
void SaveRadialMultipole(char *fn, int n, int nk, int *ks, int g) {
  int i, j, m, n1, n2, nn, k, mk, i1, i2;
  double *y;
  char s1[16], s2[16];
  FILE *f;

  if (n_awgrid <= 0) return;
  ORBITAL **orbs = malloc(sizeof(ORBITAL *)*(n_orbitals-n_continua));
  k = 0;
  for (i = 0; i < n_orbitals; i++) {
    orbs[k] = GetOrbitalSolved(i);
    if (orbs[k]->n > 0 && orbs[k]->n <= n) {
      k++;
    }
  }
  if (k <= 0) {
    free(orbs);
    return;
  }
  nn = k;
  f = fopen(fn, "w");
  if (f == NULL) {
    printf("cannot open file: %s\n", fn);
    return;
  }
  fprintf(f, "%5s %5s %4d %4d %4d %4d %2d", "###", "###",
	  (int)(potential->atom->atomic_number), (int)(potential->N),
	  n, n, n_awgrid);
  for (j = 0; j < n_awgrid; j++) {
    fprintf(f, " %15.8E", awgrid[j]);
  }
  fprintf(f, "\n");
  for (n1 = 0; n1 < nn; n1++) {
    ShellString(orbs[n1]->n, orbs[n1]->kappa, -1, s1);
    i1 = ShellToInt(orbs[n1]->n, orbs[n1]->kappa);
    for (n2 = 0; n2 < nn; n2++) {
      ShellString(orbs[n2]->n, orbs[n2]->kappa, -1, s2);
      i2 = ShellToInt(orbs[n2]->n, orbs[n2]->kappa);
      for (i = 0; i < nk; i++) {
	if (ks) {
	  k = ks[i];
	} else {
	  k = i+1;
	}
	for (mk = -1; mk <= 1; mk += 2) {      
	  m = MultipoleRadialFRGrid(&y, mk*k, orbs[n1]->idx, orbs[n2]->idx, g);
	  if (m > 0) {
	    fprintf(f, "%5s %5s %4d %4d %4d %4d %2d",
		    s1, s2, mk*k, g, i1, i2, m);
	    for (j = 0; j < m; j++) {
	      fprintf(f, " %15.8E", y[j]);
	    }
	    fprintf(f, "\n");
	  }
	}
      }
    }
  }
  free(orbs);
  fclose(f);
}

double GetHXS(POTENTIAL *p) {
  double z, hs;
  double r0, r1;
  int n;
  z = p->atom->atomic_number;
  if (p->hx0 > 0) {
    r0 = p->hx0 + p->hx1*z*z;
    r0 = Max(r0, 0.1);
    r0 = Min(r0, 0.9);
    return r0;
  }
  if (p->hx0 <= -5.0) {
    r0 = 0.42356543; // (3/(4*pi^2))**(1/3)
    return r0;
  }
  n = (int)(0.5+p->N);
  if (n < 2) return 0.5;
  hs = fabs(p->ihx);
  if (hs <= 0.001) hs = 0;
  if (hs >= 0.999) hs = 1;
  r0 = 0.5;
  r1 = 0.5;
  if (hs < 0.999) {
    if (n <= NHXN) {
      r0 = _hx0[0][n-2] + _hx1[0][n-2]*z*z;
    } else {
      r0 = _hx0[0][NHXN-1] + _hx1[0][NHXN-1]*z*z;
    }
    r0 = Max(r0, 0.1);
    r0 = Min(r0, 0.9);
    if (hs < 0.001) return r0;
  }
  if (hs > 0.001) {
    if (n <= NHXN) {
      r1 = _hx0[1][n-2] + _hx1[1][n-2]*z*z;
    } else {
      r1 = _hx0[1][NHXN-1] + _hx1[1][NHXN-1]*z*z;
    }
    r1 = Max(r1, 0.1);
    r1 = Min(r1, 0.9);
    if (hs > 0.999) return r1;
  }
  return r1*hs + r0*(1-hs);
}
  
void CopyPotential(POTENTIAL *p1, POTENTIAL *p0) {
  if (p0 == NULL) return;
  if (p1 == NULL) {
    p1 = (POTENTIAL *) malloc(sizeof(POTENTIAL));
    p1->maxrp = 0;
  }
  AllocPotMem(p1, p0->maxrp);
  double *w = p1->dws;
  memcpy(p1, p0, sizeof(POTENTIAL));
  p1->dws = w;
  memcpy(p1->dws, p0->dws, sizeof(double)*p0->nws);
  SetPotDP(p1);
}

void AllocPotMem(POTENTIAL *p, int n) {
  if (n == p->maxrp) return;
  if (p->maxrp > 0) {
    free(p->dws);
  }
  p->maxrp = n;
  p->nws = n*(36+3*NKSEP+3*NKSEP1);
  p->dws = (double *) malloc(sizeof(double)*p->nws);
  SetPotDP(p);
}

void SetPotDP(POTENTIAL *p) {
  double *x = p->dws;
  int n = p->maxrp;
  p->Z = x;
  x += n;
  p->dZ = x;
  x += n;
  p->dZ2 = x;
  x += n;
  p->rad = x;
  x += n;
  p->rho = x;
  x += n;
  p->mqrho = x;
  x += n;
  p->dr_drho = x;
  x += n;
  p->dr_drho2 = x;
  x += n;
  p->vtr = x;
  x += n;
  p->a1r = x;
  x += n;
  p->a2r = x;
  x += n;
  p->a3r = x;
  x += n;
  p->a1dr = x;
  x += n;
  p->a2dr = x;
  x += n;
  p->a3dr = x;
  x += n;
  p->Vc = x;
  x += n;
  p->dVc = x;
  x += n;
  p->dVc2 = x;
  x += n;
  p->qdist = x;
  x += n;
  p->U = x;
  x += n;
  p->dU = x;
  x += n;
  p->dU2 = x;
  x += n;
  p->W = x;
  x += n;
  p->dW = x;
  x += n;
  p->dW2 = x;
  x += n;
  p->ZVP = x;
  x += n;
  p->dZVP = x;
  x += n;
  p->dZVP2 = x;
  x += n;
  p->NPS = x;
  x += n;
  p->VPS = x;
  x += n;
  p->EPS = x;
  x += n;
  p->VXF = x;
  x += n;
  p->ICF = x;
  x += n;
  p->ZPS = x;
  x += n;
  p->dZPS = x;
  x += n;
  p->dZPS2 = x;
  int i;
  for (i = 0; i < NKSEP; i++) {
    x += n;
    p->ZSE[i] = x;
  }
  for (i = 0; i < NKSEP; i++) {
    x += n;
    p->dZSE[i] = x;
  }
  for (i = 0; i < NKSEP; i++) {
    x += n;
    p->dZSE2[i] = x;
  }
  for (i = 0; i < NKSEP1; i++) {
    x += n;
    p->VT[i] = x;
  }
  for (i = 0; i < NKSEP1; i++) {
    x += n;
    p->dVT[i] = x;
  }
  for (i = 0; i < NKSEP1; i++) {
    x += n;
    p->dVT2[i] = x;
  }
}

void AllocDWS(int n) {
  int nws = n*35;
  if (nws != _nws) {
    if (_nws > 0) free(_dws);
    _nws = nws;
    _dws = (double *) malloc(sizeof(double)*nws);
    double *p = _dws;
    _dwork = p;
    p += n;
    _dwork1 = p;
    p += n;
    _dwork2 = p;
    p += n;
    _dwork3 = p;
    p += n;
    _dwork4 = p;
    p += n;
    _dwork5 = p;
    p += n;
    _dwork6 = p;
    p += n;
    _dwork7 = p;
    p += n;
    _dwork8 = p;
    p += n;
    _dwork9 = p;
    p += n;
    _dwork10 = p;
    p += n;
    _dwork11 = p;
    p += n;
    _dwork12 = p;
    p += n;
    _dwork13 = p;
    p += n;
    _dwork14 = p;
    p += n;
    _dwork15 = p;
    p += n;
    _dwork16 = p;
    p += n;
    _dwork17 = p;
    p += n;
    _phase = p;
    p += n;
    _dphase = p;
    p += n;
    _dphasep = p;
    p += n;
    _yk = p;
    p += n;
    _zk = p;
    p += n;
    _xk = p;
    p += n;
    SetOrbitalWorkSpace(p, n);
  }
}

void AllocWorkSpace(int n) {
  AllocDWS(n);
  AllocPotMem(potential, n);
  AllocPotMem(hpotential, n);
  AllocPotMem(rpotential, n);
}

double *WorkSpace() {
  return _dws;
}

void SetOrbMap(int k, int n0, int n1) {
  if (k <= 0) k = KORBMAP;
  if (n0 <= 0) n0 = NORBMAP0;
  if (n1 <= 0) n1 = NORBMAP1;

  int i, j;
  int blocks[3] = {MULTI_BLOCK3, MULTI_BLOCK3, MULTI_BLOCK3};

  MULTI *ozn = NULL;
  if (_orbmap != NULL) {
    for (i = 0; i < _korbmap; i++) {
      free(_orbmap[i].opn);
      free(_orbmap[i].onn);
      if (i == 0) {
	MultiFreeData(_orbmap[i].ozn, NULL);
	ozn = _orbmap[i].ozn;
      }
    }
    free(_orbmap);
  }

  _korbmap = k;
  _norbmap0 = n0;
  _norbmap1 = n1;
  _orbmap = malloc(sizeof(ORBMAP)*_korbmap);  
  for (i = 0; i < _korbmap; i++) {
    _orbmap[i].ifm = 0;
    _orbmap[i].cfm = 0.0;
    _orbmap[i].xfm = 0.0;
    _orbmap[i].nmax = 1000000000;
    _orbmap[i].opn = malloc(sizeof(ORBITAL *)*_norbmap0);
    _orbmap[i].onn = malloc(sizeof(ORBITAL *)*_norbmap1);
    for (j = 0; j < _norbmap0; j++) {
      _orbmap[i].opn[j] = NULL;
    }
    for (j = 0; j < _norbmap1; j++) {
      _orbmap[i].onn[j] = NULL;
    }
    if (i == 0) {
      if (ozn == NULL) {
	_orbmap[i].ozn = malloc(sizeof(MULTI));
	MultiInit(_orbmap[i].ozn, sizeof(ORBITAL *), 3, blocks, "ozn");
      } else {
	_orbmap[i].ozn = ozn;
      }
    }
  }
}

int RestorePotential(char *fn, POTENTIAL *p) {
  BFILE *f;
  int n, i, k;

  f = BFileOpen(fn, "r", -1);
  if (f == NULL) {
    MPrintf(0, "cannot open potential file: %s\n", fn);
    return -1;
  }
  int maxrp = 0;
  n = BFileRead(&p->mode, sizeof(int), 1, f);
  n = BFileRead(&p->flag, sizeof(int), 1, f);
  n = BFileRead(&p->r_core, sizeof(int), 1, f);
  n = BFileRead(&p->nmax, sizeof(int), 1, f);
  n = BFileRead(&maxrp, sizeof(int), 1, f);
  n = BFileRead(&p->hxs, sizeof(double), 1, f);
  n = BFileRead(&p->ahx, sizeof(double), 1, f);
  n = BFileRead(&p->ihx, sizeof(double), 1, f);
  n = BFileRead(&p->rhx, sizeof(double), 1, f);
  n = BFileRead(&p->dhx, sizeof(double), 1, f);
  n = BFileRead(&p->bhx, sizeof(double), 1, f);
  n = BFileRead(&p->chx, sizeof(double), 1, f);
  n = BFileRead(&p->hx0, sizeof(double), 1, f);
  n = BFileRead(&p->hx1, sizeof(double), 1, f);
  n = BFileRead(&p->ratio, sizeof(double), 1, f);
  n = BFileRead(&p->asymp, sizeof(double), 1, f);
  n = BFileRead(&p->rmin, sizeof(double), 1, f);
  n = BFileRead(&p->N, sizeof(double), 1, f);
  n = BFileRead(&p->N1, sizeof(double), 1, f);
  n = BFileRead(&p->lambda, sizeof(double), 1, f);
  n = BFileRead(&p->a, sizeof(double), 1, f);
  n = BFileRead(&p->ar, sizeof(double), 1, f);
  n = BFileRead(&p->br, sizeof(double), 1, f);
  n = BFileRead(&p->ib, sizeof(int), 1, f);
  n = BFileRead(&p->ib0, sizeof(int), 1, f);
  n = BFileRead(&p->nb, sizeof(int), 1, f);
  n = BFileRead(&p->ib1, sizeof(int), 1, f);
  n = BFileRead(&p->bqp, sizeof(double), 1, f);
  n = BFileRead(&p->rb, sizeof(double), 1, f);
  n = BFileRead(&p->mps, sizeof(int), 1, f);
  n = BFileRead(&p->zps, sizeof(double), 1, f);
  n = BFileRead(&p->nps, sizeof(double), 1, f);
  n = BFileRead(&p->tps, sizeof(double), 1, f);
  n = BFileRead(&p->rps, sizeof(double), 1, f);
  n = BFileRead(&p->dps, sizeof(double), 1, f);
  n = BFileRead(&p->aps, sizeof(double), 1, f);
  n = BFileRead(&p->fps, sizeof(double), 1, f);
  n = BFileRead(&p->ups, sizeof(double), 1, f);
  n = BFileRead(&p->xps, sizeof(double), 1, f);
  n = BFileRead(&p->jps, sizeof(double), 1, f);
  n = BFileRead(&p->ips, sizeof(int), 1, f);
  n = BFileRead(&p->sturm_idx, sizeof(double), 1, f);
  AllocPotMem(p, maxrp);
  n = BFileRead(p->dws, sizeof(double), p->nws, f);
  p->atom = GetAtomicNucleus();
  BFileClose(f);
  ReinitRadial(1);
  ClearOrbitalTable(0);
  SetPotentialVT(p);
  SetReferencePotential(hpotential, p, 1);
  SetReferencePotential(rpotential, p, 0);
  CopyPotentialOMP(0);
  return 0;
}

void SetReferencePotential(POTENTIAL *h, POTENTIAL *p, int hlike) {
  int i, k;
  CopyPotential(h, p);
  if (hlike) {
    h->N = 1.0;
    h->N1 = 0.0;
    h->a = 0;
    h->lambda = 0;
    h->hlike = 1;
  } else {
    h->hlike = 0;
  }
  h->sps = 1;
  h->nse = 0;
  h->mse = 0;
  h->pse = 0;
  h->mvp = 0;
  h->pvp = 0;
  for (i = 0; i < p->maxrp; i++) {
    if (hlike) {
      h->Vc[i] = 0;
      h->dVc[i] = 0;
      h->dVc2[i] = 0;
      h->qdist[i] = 0;
      h->U[i] = 0;
      h->dU[i] = 0;
      h->dU2[i] = 0;
      h->W[i] = 0;
      h->dW[i] = 0;
      h->dW2[i] = 0;
      if (h->atom->nep > 0 || h->atom->nepr > 0) {
	h->Z[i] = GetAtomicEffectiveZ(h->rad[i]);
      }
    }
    h->ZVP[i] = 0;
    h->dZVP[i] = 0;
    h->dZVP2[i] = 0;
    for (k = 0; k < NKSEP; k++) {
      h->ZSE[k][i] = 0;
      h->dZSE[k][i] = 0;
      h->dZSE2[k][i] = 0;
    }
  }
  if (hlike) {
    if (h->atom->nep > 0 || h->atom->nepr > 0) {
      Differential(h->Z, h->dZ, 0, h->maxrp-1, h);
      Differential(h->dZ, h->dZ2, 0, h->maxrp-1, h);
    }
    SetPotentialVc(h);
  }
  SetPotentialVT(h);
}

int SavePotential(char *fn, POTENTIAL *p) {
  FILE *f;
  int n, i;

  if (MyRankMPI() != 0) return 0;
  
  f = fopen(fn, "w");
  if (f == NULL) {
    MPrintf(0, "cannot open potential file: %s\n", fn);
    return -1;
  }

  n = fwrite(&p->mode, sizeof(int), 1, f);
  n = fwrite(&p->flag, sizeof(int), 1, f);
  n = fwrite(&p->r_core, sizeof(int), 1, f);
  n = fwrite(&p->nmax, sizeof(int), 1, f);
  n = fwrite(&p->maxrp, sizeof(int), 1, f);
  n = fwrite(&p->hxs, sizeof(double), 1, f);
  n = fwrite(&p->ahx, sizeof(double), 1, f);
  n = fwrite(&p->ihx, sizeof(double), 1, f);
  n = fwrite(&p->rhx, sizeof(double), 1, f);
  n = fwrite(&p->dhx, sizeof(double), 1, f);
  n = fwrite(&p->bhx, sizeof(double), 1, f);
  n = fwrite(&p->chx, sizeof(double), 1, f);
  n = fwrite(&p->hx0, sizeof(double), 1, f);
  n = fwrite(&p->hx1, sizeof(double), 1, f);
  n = fwrite(&p->ratio, sizeof(double), 1, f);
  n = fwrite(&p->asymp, sizeof(double), 1, f);
  n = fwrite(&p->rmin, sizeof(double), 1, f);
  n = fwrite(&p->N, sizeof(double), 1, f);
  n = fwrite(&p->N1, sizeof(double), 1, f);
  n = fwrite(&p->lambda, sizeof(double), 1, f);
  n = fwrite(&p->a, sizeof(double), 1, f);
  n = fwrite(&p->ar, sizeof(double), 1, f);
  n = fwrite(&p->br, sizeof(double), 1, f);
  n = fwrite(&p->ib, sizeof(int), 1, f);
  n = fwrite(&p->ib0, sizeof(int), 1, f);
  n = fwrite(&p->nb, sizeof(int), 1, f);
  n = fwrite(&p->ib1, sizeof(int), 1, f);
  n = fwrite(&p->bqp, sizeof(double), 1, f);
  n = fwrite(&p->rb, sizeof(double), 1, f);
  n = fwrite(&p->mps, sizeof(int), 1, f);
  n = fwrite(&p->zps, sizeof(double), 1, f);
  n = fwrite(&p->nps, sizeof(double), 1, f);
  n = fwrite(&p->tps, sizeof(double), 1, f);
  n = fwrite(&p->rps, sizeof(double), 1, f);
  n = fwrite(&p->dps, sizeof(double), 1, f);
  n = fwrite(&p->aps, sizeof(double), 1, f);
  n = fwrite(&p->fps, sizeof(double), 1, f);
  n = fwrite(&p->ups, sizeof(double), 1, f);
  n = fwrite(&p->xps, sizeof(double), 1, f);
  n = fwrite(&p->jps, sizeof(double), 1, f);
  n = fwrite(&p->ips, sizeof(int), 1, f);
  n = fwrite(&p->sturm_idx, sizeof(double), 1, f);
  n = fwrite(p->dws, sizeof(double), p->nws, f);
  fclose(f);
  return 0;
}

int ModifyPotential(char *fn, POTENTIAL *p) {
  BFILE *f;
  int n, i, k, np;
  char buf[BUFLN];
  char *c;

  f = BFileOpen(fn, "r", -1);
  if (f == NULL) {
    MPrintf(0, "cannot open potential file: %s\n", fn);
    return -1;
  }
  i = 0;
  while (1) {
    if (NULL == BFileGetLine(buf, BUFLN, f)) break;
    buf[BUFLN-1] = '\0';
    c = buf;
    while (*c && (*c == ' ' || *c == '\t')) c++;
    if (*c == '\0' || *c == '#') continue;
    if (i >= p->maxrp) {
      MPrintf(0, "potential file exceeds max grid points: %d %d\n",
	      i, p->maxrp);
      break;
    }
    k = sscanf(buf, "%lg %lg", _dwork12+i, _dwork13+i);
    if (k != 2) continue;    
    i++;
  }
  BFileClose(f);
  n = i;  
  for (i = 0; i < n; i++) {
    _dwork13[i] *= _dwork12[i];
    _dwork12[i] = log(_dwork12[i]);
  }
  for (i = 0; i < p->maxrp; i++) {
    p->W[i] = log(p->rad[i]);
  }
  np = 3;
  UVIP3P(np, n, _dwork12, _dwork13, p->maxrp, p->W, p->dW);
  for (i = 0; i < p->maxrp; i++) {
    p->U[i] = p->dW[i]/p->rad[i] - p->Vc[i];
  }
  SetPotentialU(p, p->maxrp, NULL);
  ReinitRadial(1);
  return 0;
}

static void InitFltAryData(void *p, int n) {
  FLTARY *d;
  int i;

  d = (FLTARY *) p;
  for (i = 0; i < n; i++) {
    d[i].npts = -1;
    d[i].yk = NULL;
  }
}

int FreeSimpleArray(MULTI *ma) {
  MultiFreeData(ma, NULL);
  return 0;
}

int FreeSlaterArray(void) {
  FreeSimpleArray(slater_array);
  return 0;
}

int FreeResidualArray(void) {
  return FreeSimpleArray(residual_array);
}

static void FreeMultipole(void *p) {
  double *dp;
  dp = *((double **) p);
  free(dp);
  *((double **) p) = NULL;
}

static void FreeFltAryData(void *p) {
  FLTARY *dp;
  
  dp = (FLTARY *) p;
  if (dp->npts > 0) {
    free(dp->yk);    
    dp->yk = NULL;
    dp->npts = -1;
  }
}

int FreeMultipoleArray(void) {
  MultiFreeData(multipole_array, FreeMultipole);
  LoadRadialMultipole(NULL);
  return 0;
}

int FreeMomentsArray(void) {
  MultiFreeData(moments_array, NULL);
  return 0;
}

int FreeGOSArray(void) {
  MultiFreeData(gos_array, FreeMultipole);
  return 0;
}

int FreeVintiArray(void) {
  MultiFreeData(vinti_array, FreeMultipole);
  return 0;
}

int FreeBreitArray(void) {
  int i;
  FreeSimpleArray(breit_array);
  FreeSimpleArray(wbreit_array);
  for (i = 0; i < 5; i++) {
    MultiFreeData(xbreit_array[i], FreeFltAryData);
  }
  return 0;
}

int FreeYkArray(void) {
  MultiFreeData(yk_array, FreeFltAryData);
  return 0;
}
  
double *WLarge(ORBITAL *orb) {
  return Large(orb);
}

double *WSmall(ORBITAL *orb) {
  return Small(orb);
}

POTENTIAL *RadialPotential(void) {
  return potential;
}

void SetSlaterCut(int k0, int k1) {
  if (k0 > 0) {
    slater_cut.kl0 = 2*k0;
  } else {
    slater_cut.kl0 = 1000000;
  } 
  if (k1 > 0) {
    slater_cut.kl1 = 2*k1;
  } else {
    slater_cut.kl1 = 1000000;
  }
}

void SolvePseudo(int kmin, int kmax, int nb, int nmax, int nd, double xdf) {
  int k, n, i, k2, j, na, ka, kb, kn, maxrp2;
  double *p, *q, pn, qn, a, e0;
  ORBITAL *orb0, *orb;
  
  if (nb <= 0) nb = potential->nb;
  if (nb <= 0) {
    printf("invalid nb in SolvePseudoOrbitals: %d\n", nb);
    return;
  }
  potential->npseudo = nb;
  potential->mpseudo = nmax;
  if (nd == -1) nd = nb;
  else if (nd == -2) nd = nmax;
  else if (nd == 0) nd = 1;
  potential->dpseudo = nd;
  maxrp2 = 2*potential->maxrp;
  ResetWidMPI();
#pragma omp parallel default(shared) private(k, k2, na, j, ka, kb, orb0, n, kn, orb, p, q, a, e0, pn, qn)
  {
  for (k = kmin; k <= kmax; k++) {
    k2 = 2*k;
    na = k + nd;
    if (na < nb) na = nb;
    if (na > nmax) na = nmax;
    for (j = k2-1; j <= k2+1; j += 2) {
      if (j < 0) continue;
      int skip = SkipMPI();
      if (skip) continue;
      double wt0 = WallTime();
      ka = GetKappaFromJL(j, k2);
      for (n = k+1; n <= na; n++) {
	kb = OrbitalIndex(n, ka, 0);
      }
      orb0 = GetOrbitalSolved(kb);
      double wt1 = WallTime();
      for (n = na+1; n <= nmax; n++) {
	kn = OrbitalIndex(n, ka, 0);
	orb = GetOrbital(kn);
	if (orb->isol <= 0) {
	  orb->wfun = malloc(sizeof(double)*maxrp2);	  
	}
	if (orb->isol != 3) {
	  orb->isol = 3;
	  memcpy(orb->wfun, orb0->wfun, sizeof(double)*maxrp2);
	  p = Large(orb);
	  q = Small(orb);
	  orb->bqp0 = orb0->bqp0;
	  orb->bqp1 = orb0->bqp1;
	  //orb->im = orb0->im;
	  orb->qr_norm = orb0->qr_norm;
	  orb->ilast = orb0->ilast;
	  orb->kv = orb0->kv;
	  for (i = 0; i <= orb->ilast; i++) {
	    a = potential->rad[i]/potential->rad[orb->ilast];
	    p[i] *= sin(TWO_PI*a);
	  }
	  e0 = orb0->energy;
	  orb->energy = e0;
	  DiracSmall(orb, potential, -1, orb->kv);
	  pn = InnerProduct(0, orb->ilast, p, p, potential);
	  qn = InnerProduct(0, orb->ilast, q, q, potential);
	  a = 1.0/sqrt(pn+qn);
	  for (i = 0; i <= orb->ilast; i++) {
	    p[i] *= a;
	    q[i] *= a;
	  }	  
	  Orthogonalize(orb);
	}
	orb0 = orb;
      }
      double wt2 = WallTime();
      if (xdf >= 0) SolveDFKappa(ka, nmax, xdf);
      double wt3 = WallTime();
      MPrintf(-1, "SolvePseudo: %3d %3d %2d %g %11.4E %11.4E %11.4E\n",
	      na, nmax, ka, xdf, wt1-wt0, wt2-wt1, wt3-wt2);
    }
  }
  }
}

void SolveDFKappa(int ka, int nmax, double xdf) {
  int j, k, k2, n, i, isym, m, s, t, nn, maxrp2;
  int ig, ic;
  double *mix;
  ORBITAL **orb;
  double **wf, r, r0, ec;
  HAMILTON *h;  
  AVERAGE_CONFIG *acfg;
  CONFIG *c;
  CONFIG_GROUP *g;

  acfg = &average_config;
  
  GetJLFromKappa(ka, &j, &k2);
  k = k2/2;
  maxrp2 = potential->maxrp*2;
  nn = nmax-k;
  orb = malloc(sizeof(ORBITAL *)*nn);
  wf = malloc(sizeof(double *)*nn);
  isym = j*2;
  if (IsOdd(k)) isym++;
  h = GetHamilton(isym);
  AllocHamMem(h, nn, nn);
  i = 0;
  for (n = k+1; n <= nmax; n++,i++) {
    m = OrbitalIndex(n, ka, 0);
    orb[i] = GetOrbitalSolved(m);
    wf[i] = malloc(sizeof(double)*maxrp2);
    memcpy(wf[i], orb[i]->wfun, sizeof(double)*maxrp2);
    h->basis[i] = n;
  }
  for (j = 0; j < nn; j++) {
    t = j*(j+1)/2;
    for (i = 0; i <= j; i++) {
      r0 = ZerothHamilton(orb[i], orb[j]);
      r = 0;
      if (xdf >= 0) {
	for (ig = 0; ig < acfg->ng; ig++) {
	  g = GetGroup(acfg->kg[ig]);
	  double tec = 0;
	  for (ic = 0; ic < g->n_cfgs; ic++) {
	    c = GetConfigFromGroup(acfg->kg[ig], ic);
	    ec = ConfigHamilton(c, orb[i], orb[j], xdf);
	    if (i == j) {
	      ec += AverageEnergyConfig(c);
	    }
	    tec += ec;
	  }
	  if (g->n_cfgs > 0) {
	    r += acfg->weight[ig]*tec/g->n_cfgs;
	  }
	}
      }
      h->hamilton[i+t] = r0 + r;
      //printf("dh: %d %d %12.5E %12.5E %12.5E\n", orb[i]->n, orb[j]->n, r0, r, h->hamilton[i+t]);
    }
  }
  
  if (0 > DiagnolizeHamilton(h)) {
    MPrintf(-1, "Diag DF Ham error: %d %d\n", ka, nmax);
    Abort(1);
  }
  
  mix = h->mixing + h->dim;
  for (i = 0; i < h->dim; i++) {
    orb[i]->energy = h->mixing[i];
    for (s = 0; s < maxrp2; s++) {
      orb[i]->wfun[s] = 0;
    }

    if (mix[i] < 0) {
      for (t = 0; t < h->dim; t++) {
	mix[t] = -mix[t];
      }
    }

    for (t = 0; t < h->dim; t++) {
      for (s = 0; s < maxrp2; s++) {
	orb[i]->wfun[s] += wf[t][s]*mix[t];
      }
    }
    mix += h->dim;
  }
  for (i = 0; i < h->dim; i++) {
    free(wf[i]);
  }
  free(wf);
  free(orb);
  AllocHamMem(h, -1, -1);
  AllocHamMem(h, 0, 0);
  ReinitRadial(2);
}

void SetPotentialMode(int m, double h, double ih,
		      double dh, double h0, double h1) {
  potential->mode = m;
  if (h > 1e10) {
    potential->hxs = POTHXS;
  } else {
    potential->hxs = h;
  }
  if (ih > 1e10) {
    potential->ihx = POTIHX;
  } else {
    potential->ihx = ih;
  }
  if (dh <= 0) {
    potential->bhx = POTBHX;
  } else {
    potential->bhx = dh;
  } 
  potential->hx0 = h0;
  potential->hx1 = h1;
}

void PrintQED() {
  MPrintf(0, "SE: %d %d %d %d\n",
	  qed.se, qed.mse, potential->pse, qed.sse);
  MPrintf(0, "MSE: %g %g %g %g %g %g\n",
	  qed.ose0, qed.ose1, qed.ase, qed.cse0, qed.cse1, qed.ise);
  MPrintf(0, "VP: %d %d\n", qed.vp, potential->pvp);
  MPrintf(0, "MS: %d %d\n", qed.nms, qed.sms);
  MPrintf(0, "BR: %d %d %d %g %d\n",
	  qed.br, qed.mbr, qed.nbr, qed.xbr, qed.minbr);
}

int GetBoundary(double *rb, double *b, int *nmax, double *dr) {
  int ib;

  if (potential->ib > 0) {
    ib = potential->ib;
  } else {
    ib = potential->maxrp-1;
  }
  *rb = potential->rad[ib];
  *b = potential->bqp;
  *nmax = potential->nb;
  *dr = potential->rad[ib]-potential->rad[ib-1];
  return potential->ib;
}

int SetBoundaryMaster(int nmax, double p, double bqp, double rf) {
  ORBITAL *orb;
  int i, j, n, kl, kl2, kappa, k;
  double d1, d2, d;

  if (nmax == -100) {
    if (p <= 0.0) {
      printf("2nd argument must be > 0 in SetBoundary(-100,...)\n");
      return -1;
    }
    for (i = 0; i < potential->maxrp-10; i++) {
      if (potential->rad[i] >= p) break;
    }
    potential->ib1 = i;
    if (bqp > 0) {
      if (bqp >= p) {
	printf("3rd argument must be less than 2nd in SetBoundary(-100,...)\n");
	return -1;
      }
      for (i = 0; i < potential->maxrp; i++) {
	if (potential->rad[i] >= bqp) break;
      }
      if (i < potential->ib1) {
	potential->ib = i;
	potential->ib0 = i;
      }
    }
    return 0;
  }
  potential->nb = abs(nmax);
  potential->bqp = bqp;
  if (nmax == 0) {
    potential->ib = 0;
    potential->ib0 = 0;
  } else if (nmax < 0) {
    d = GetResidualZ();
    d1 = potential->nb;
    if (p < 0.0) p = 2.0*d1*d1/d;
    for (i = 0; i < potential->maxrp; i++) {
      if (potential->rad[i] >= p) break;
    }
    if (i > potential->maxrp-10) {
      printf("enlarge maxrp0: %d %d\n", i, potential->maxrp);
      exit(1);
    }
    if (IsEven(i)) i++;
    potential->ib = i;
    potential->ib0 = 0;
    for (n = 1; n <= potential->nb; n++) {
      for (kl = 0; kl < n; kl++) {
	kl2 = 2*kl;
	for (j = kl2 - 1; j <= kl2 + 1; j += 2) {
	  if (j < 0) continue;
	  kappa = GetKappaFromJL(j, kl2);
	  k = OrbitalIndex(n, kappa, 0);
	  orb = GetOrbitalSolved(k);
	}
      }
    }
  } else {
    if (p < 0.0) p = 1E-8;
    for (n = 1; n <= potential->nb; n++) {
      for (kl = 0; kl < n; kl++) {
	kl2 = 2*kl;
	for (j = kl2 - 1; j <= kl2 + 1; j += 2) {
	  if (j < 0) continue;
	  kappa = GetKappaFromJL(j, kl2);
	  k = OrbitalIndex(n, kappa, 0);
	  orb = GetOrbitalSolved(k);	  
	  for (i = orb->ilast-3; i >= 0; i--) {
	    d1 = Large(orb)[i];
	    d2 = Small(orb)[i];
	    d = d1*d1 + d2*d2;
	    if (d >= p) {
	      i++;
	      break;
	    }
	  }
	  if (potential->ib < i) potential->ib = i;
	}
      }
    }
    if (IsEven(potential->ib)) potential->ib++;
    potential->ib0 = potential->ib;
    if (rf > 0) {
      i = potential->ib;
      k = OrbitalIndex(nmax, -1, 0);
      orb = GetOrbitalSolved(k);
      double *w = Large(orb);
      for (j = i; j > 5; j--) {
	if ((w[j-1] > w[j] && w[j-2] <= w[j-1]) ||
	    (w[j-1] < w[j] && w[j-2] >= w[j-1])) {
	  j--;
	  break;
	}
      }
      int jm = j;
      for (; j > 0; j--) {
	if ((w[jm] > 0 && w[j] <= 0) ||
	    (w[jm] < 0 && w[j] >= 0)) {
	  break;
	}
      }
      d1 = potential->rad[i]+rf*(potential->rad[jm]-potential->rad[j]);
      for (; i < potential->maxrp-5; i++) {
	if (potential->rad[i] >= d1) break;
      }    
      if (IsEven(i)) i++;
      potential->ib = i;      
      if (i > potential->maxrp-5) {
	printf("enlarge maxrp1: %d %d\n", i, potential->maxrp);
	return -1;
      }
    }
  }
  if (potential->ib > 0 && potential->ib < potential->maxrp) {
    potential->rb = potential->rad[potential->ib];
    if (_sturm_idx > 0) {
      potential->sturm_idx = _sturm_idx;
      SetPotentialSturm(potential);
    }
  }
  potential->ib1 = potential->ib;
  return 0;
}

int SetBoundary(int nmax, double p, double bqp, double rf,
		int nri, int ng, int *kg, char *s,
		int n0, int n1, int n0d, int n1d, int k0, int k1) {
  int r = SetBoundaryMaster(nmax, p, bqp, rf);
  int nr = abs(nri);
  if (nr > 0 && rf > 0 && potential->ib > potential->ib0) {
    int nr0 = potential->ib-potential->ib0+1;
    if (nr > nr0) nr = nr0;
    double mde = 0;
    int mib = -1;
    int i, k, ib, ib1;
    int ig, ic;
    CONFIG_GROUP *g;
    CONFIG *c;
    k = nr0/nr;
    k = Max(1, k);
    ib = potential->ib0;
    ib1 = potential->ib;
    for (i = 0; i < nr; i++) {
      if (ib > ib1) break;
      potential->ib = ib;
      potential->ib1 = ib;
      potential->rb = potential->rad[potential->ib];
      ReinitRadial(1);
      ClearOrbitalTable(0);      
      for (ig = 0; ig < ng; ig++) {
	g = GetGroup(kg[ig]);
	for (ic = 0; ic < g->n_cfgs; ic++) {
	  c = GetConfigFromGroup(kg[ig], ic);
	  c->mde = 1E31;
	}
      }
      ConfigSD(43, ng, kg, s, NULL, NULL, n0, n1, n0d, n1d, k0, k1, 1, NULL, 0);
      double de = 1e31;
      for (ig = 0; ig < ng; ig++) {
	g = GetGroup(kg[ig]);
	for (ic = 0; ic < g->n_cfgs; ic++) {
	  c = GetConfigFromGroup(kg[ig], ic);
	  if (c->mde < de) de = c->mde;
	}
      }
      if (de > 9e30) de = 0;
      if (de > mde) {
	mde = de;
	mib = ib;
      }
      if (nri < 0) {
	printf("adjust boundary: %4d %4d %4d %4d %4d %4d %10.4E %10.4E %10.4E\n",
	       i, potential->ib0, ib1, k, ib, mib, de, mde, TotalSize());
	fflush(stdout);
      }
      ib += k;
    }
    potential->ib = mib;
    potential->ib1 = mib;    
    potential->rb = potential->rad[potential->ib];
    ReinitRadial(1);
    ClearOrbitalTable(0);
    for (ig = 0; ig < ng; ig++) {
      g = GetGroup(kg[ig]);
      for (ic = 0; ic < g->n_cfgs; ic++) {
	c = GetConfigFromGroup(kg[ig], ic);
	c->mde = 1E31;
      }
    }
    ConfigSD(43, ng, kg, s, NULL, NULL, n0, n1, n0d, n1d, k0, k1, 1, NULL, 0);
    ReinitRadial(1);
    ClearOrbitalTable(0);
  }
  CopyPotentialOMP(0);
  return r;
}

int RadialOverlaps(char *fn, int kappa) {
  ORBITAL *orb1, *orb2;
  int i, j, k;
  double r, b1, b2, a1, a2;
  FILE *f;

  f = fopen(fn, "w");
  for (k = 0; k < potential->maxrp; k++) {
    _dwork10[k] = 1.0;
  }
  for (i = 0; i < n_orbitals; i++) {
    orb1 = GetOrbital(i);
    if (orb1->kappa != kappa) continue;
    a1 = ZerothHamilton(orb1, orb1);
    for (j = 0; j < n_orbitals; j++) {
      orb2 = GetOrbital(j);
      if (orb2->kappa != kappa) continue;
      a2 = ZerothHamilton(orb2, orb2);
      Integrate(_dwork10, orb1, orb2, 1, &r, 0);
      b1 = ZerothHamilton(orb1, orb2);
      b2 = ZerothHamilton(orb2, orb1);
      fprintf(f, "%2d %2d %12.5E %12.5E  %2d %2d %12.5E %12.5E %12.5E %12.5E %12.5E\n",
	      orb1->n, orb1->kappa, orb1->energy, a1, 
	      orb2->n, orb2->kappa, orb2->energy, a2, r, b1, b2);
    }
  }
  fclose(f);

  return 0;
}
  
void SetSE(int n, int m, int s, int p) {
  qed.se = n;
  if (m >= 0) qed.mse = m%100;
  if (s >= 0) qed.sse = s;
  if (p >= 0) qed.pse = p;
  if (m >= 1000) {
    potential->hpvs = 1;
    m = m%1000;
  } else {
    potential->hpvs = 0;
  }
  if (m >= 140) {
    potential->pse = 1;
  } else {
    potential->pse = 0;
  }
  potential->mse = qed.mse;
  potential->nse = qed.se;
}

void SetModSE(double ose0, double ose1, double ase,
	      double cse0, double cse1, double ise) {
  qed.ose0 = ose0;
  qed.ose1 = ose1;
  qed.ase = ase;
  if (cse0 > 0) {
    qed.cse0 = cse0;
  }
  if (cse1 > 0) {
    qed.cse1 = cse1;
  }
  if (ise > 0) {
    qed.ise = ise;
  }
}

void SetVP(int n) {
  qed.vp = n;
  potential->mvp = qed.vp%100;
  potential->pvp = qed.vp > 100;
}

int GetBreit(void) {
  return qed.br;
}

void SetBreit(int n, int m, int n0, double x0, int n1) {
  qed.br = n;
  if (m >= 0) qed.mbr = m;
  if (n0 >= 0) qed.nbr = n0;
  if (x0 > 0) qed.xbr = x0;
  if (n1 >= 0) qed.minbr = n1;
}

void SetMS(int nms, int sms) {
  qed.nms = nms;
  qed.sms = sms;
}

int SetAWGrid(int n, double awmin, double awmax) {
  int i;

  if (n == -1) {
    n_awgrid = 1;
    uta_awgrid = 1;
    awgrid[0] = awmin;
    return 0;
  }
  uta_awgrid = 0;
  if (awmin < _awmin) {
    awmax = awmax + (_awmin-awmin);
    awmin = _awmin;
  }
  if (n > MAXNAW) {
    printf("naw exceeded max: %d %d\n", n, MAXNAW);
    n = MAXNAW;
  }
  double d = (awmax-awmin)/(n-1);
  awgrid[0] = awmin;
  for (i = 1; i < n; i++) {
    awgrid[i] = awgrid[i-1] + d;
  }
  n_awgrid = n;
  return 0;
}

int GetAWGrid(double **a) {
  *a = awgrid;
  return n_awgrid;
}

int OptimizeMaxIter(void) {
  return optimize_control.maxiter;
}

void SetOptimizeMaxIter(int m) {
  optimize_control.maxiter = m;
}

void SetOptimizeStabilizer(double m) {
  if (m < 0) {
    optimize_control.iset = 0;
  } else {
    optimize_control.iset = 1;
    optimize_control.stabilizer = m;
  }
}

void SetOptimizeTolerance(double c) {
  optimize_control.tolerance = c;
}

void SetOptimizePrint(int m) {
  optimize_control.iprint = m;
}

void SetConfigEnergyMode(int m) {
  optimize_control.mce = m;
}

int ConfigEnergyMode() {
  return optimize_control.mce;
}

void SetOptimizeControl(double tolerance, double stabilizer, 
			int maxiter, int iprint) {
  optimize_control.maxiter = maxiter;
  optimize_control.stabilizer = stabilizer;
  optimize_control.tolerance = tolerance;
  optimize_control.iprint = iprint;  
  optimize_control.iset = 1;
}

void SetScreening(int n_screen, int *screened_n, 
		  double screened_charge, int kl) {
  optimize_control.screened_n = screened_n;
  optimize_control.screened_charge = screened_charge;
  optimize_control.n_screen = n_screen;
  optimize_control.screened_kl = kl;
}

int SetRadialGrid(int maxrp, double ratio, double asymp,
		  double rmin, double qr) {
  if (maxrp < 0) maxrp = _maxrp;
  AllocWorkSpace(maxrp);
  if (asymp < 0 && ratio < 0) {
    asymp = _gridasymp;
    ratio = _gridratio;
  }
  if (rmin <= 0) rmin = _gridrmin;
  potential->rmin = rmin;
  if (ratio == 0) potential->ratio = _gridratio;
  else potential->ratio = ratio;
  if (asymp == 0) potential->asymp = _gridasymp;
  else potential->asymp = asymp;
  if (qr <= 0) potential->qr = _gridqr;
  else potential->qr = qr;
  potential->flag = 0;
  potential->mps = -1;
  potential->vxf = 0;
  potential->zps = 0;
  potential->nps = 0;
  potential->tps = 0;
  potential->ups = 0;
  potential->rps = 0;
  potential->cps = 0;
  potential->dps = 0;
  potential->aps = 0;
  potential->fps = 0;
  potential->ips = 0;
  potential->sps = 0;
  potential->bps = 0;
  potential->nbs = 0;
  potential->nqf = 0;
  potential->iqf = 0;
  potential->ewd = 0;
  potential->sturm_idx = _sturm_idx;
  int i;
  for (i = 0; i < NKSEP1; i++) {
    potential->sturm_ene[i] = 0.0;
  }
  return 0;  
}

void AdjustScreeningParams(double *u) {
  int i;
  double c;
  
  c = 0.5*u[potential->maxrp-1];
  for (i = 0; i < potential->maxrp; i++) {
    if (u[i] > c) break;
  }
  potential->lambda = log(2.0)/potential->rad[i];
}

int PotentialHX(AVERAGE_CONFIG *acfg, double *u, int iter) {
  int md, md1, jmax, j, i, m, jm, np;
  double *u0, *ue, *ut, *ux;
  double a, b, n0, n1, n2, ur, r2;
  
  if (acfg->n_cores <= 0 && potential->mps < 0 && _scpot.nr == 0) {
    for (i = 0; i < potential->maxrp; i++) {
      potential->NPS[i] = 0.0;
      potential->EPS[i] = 0.0;
      potential->VPS[i] = 0.0;
      potential->VXF[i] = 0.0;
    }
    return 0;
  }
  md = potential->mode % 10;
  md1 = potential->mode / 10;
  ut = _dwork14;
  ue = _dwork15;
  u0 = _dwork16;
  ux = _dwork17;
  for (i = 0; i < potential->maxrp; i++) {
    ue[i] = 0;
    u0[i] = 0;
    ut[i] = 0;
    u[i] = 0;
  }  
  jmax = PotentialHX1(acfg, iter, md);
  for (i = 0; i < potential->maxrp; i++) {
    ue[i] = _dphase[i];
    u0[i] = _dphasep[i];
  }  
  if (jmax <= 0 || potential->N+potential->nqf < 1+EPS5) {
    for (m = 0; m < potential->maxrp; m++) {
      u[m] = 0.0;
      potential->ZPS[m] = 0;
      ut[m] = 0;
      potential->r_core = 0;
    }
    return 0;
  }
  if (potential->mps <= 2 ) {
    b = potential->N/potential->atom->atomic_number;
    if (1+potential->N1 == 1) {
      for (m = 0; m < potential->maxrp; m++) {
	u[m] = 0.0;
	ue[m] = 0.0;
	ut[m] = 0.0;
      }
    }
  } else {
    b = (potential->N+potential->nqf)/potential->atom->atomic_number;
  }
  b = Max(1.0, b);
  //printf("mps: %d %g %g\n", potential->mps, potential->N1, b);
  for (m = 0; m < potential->maxrp; m++) {
    u[m] = u0[m] - ue[m];
    if (potential->mps <= 2) {
      ut[m] = u[m];
    } else {
      ut[m] = u[m] + potential->ZPS[m];
    }
    a = b*potential->Z[m];
    if (ut[m] > a) {
      ur = ut[m] - a;
      u[m] -= ur*(u[m]/ut[m]);
      if (potential->mps > 2) {
	potential->ZPS[m] -= ur*(potential->ZPS[m]/ut[m]);
      }
      ut[m] = a;
    }
  }
  _jsi = 0;
  if (_csi > -1 && fabs(_csi) > 1e-10 &&
      potential->ihx < 0 && (_scpot.md < 0 || _scpot.md > 1)) {
    if (_csi > 0) {
      n0 = potential->N;
      if (potential->mps > 2) n0 += potential->nqf;
      n1 = n0 + potential->ihx;
      n2 = n0 + (1+_csi)*potential->ihx;
      n1 = Max(0, n1);
      n2 = Max(0, n2);
      jm = jmax;
      for (m = jmax; m > 0; m--) {
	if (ut[m-1] < ut[m]) {
	  jm = m;
	  break;
	}
      }
      for (m = jm+1; m <= jmax; m++) {
	ut[m] = ut[jm];
      }
      for (m = jm; m > 0; m--) {
	if (ut[m] <= n1) {
	  i = m;
	  break;
	}
      }
      for (m = i; m > 0; m--) {
	if (ut[m] < n2) {
	  j = m;
	  break;
	}
      }
      _jsi = 0;
      //printf("n12: %g %g %g %g %d\n", n0, n1, n2, _opm_ahx, j);
      if (j < 10) {
	for (m = 0; m <= jmax; m++) {
	  u[m] = 0.0;
	  ut[m] = 0.0;
	  if (potential->mps > 2) {
	    potential->ZPS[m] = 0.0;
	  }
	}
      } else if (j < jm) {
	_jsi = j;
	a = 0.0;
	b = 0.0;
	ur = ut[j];
	for (m = j-3; m <= j+3; m++) {
	  if (m < jmax && m != j) {
	    r2 = potential->rad[m] - potential->rad[j];
	    a += -(log(1-(ut[m]-ur)/(n0-ur)))/r2;
	    b += 1.0;
	  }
	}
	a /= b;
	b = 0.0;
	b = a*(n0-ur)/(n1-ur);
	r2 = potential->rad[j];
	//printf("r2: %d %g %g %g %g %g %g %g %g\n", j, potential->rad[j], r2, n0, n1, n2, ur, a, b);
	for (m = j+1; m <= jmax; m++) {
	  ux[m] = ur + (n1-ur)*(1-exp(-b*(potential->rad[m]-r2)))-ut[m];
	  u[m] += ux[m]*(u[m]/ut[m]);
	  if (potential->mps > 2) {
	    potential->ZPS[m] += ux[m]*(potential->ZPS[m]/ut[m]);
	  }
	  ut[m] += ux[m];
	}
      } else {
	for (m = jm+1; m <= jmax; m++) {
	  ur = ut[jm];
	  u[m] += (ur-ut[m])*(u[m]/ut[m]);
	  if (potential->mps > 2) {
	    potential->ZPS[m] += (ur-ut[m])*(potential->ZPS[m]/ut[m]);
	  }
	  ut[m] = ur;
	}
      }
    } else {
      n0 = ut[jmax];
      n1 = (n0+potential->ihx)/n0;
      r2 = n1 + _csi/n0;
      b = _xhx*(n1-r2);
      ur = (1+1./b)/2.0;
      for (j = jmax; j >= 0; j--) {
	n2 = r2-ut[j]/n0;
	a = n2/b;
	if (a > 0) {
	  if (a > 35.0) break;
	  a = 1.0/(1.0+exp(a));
	} else {
	  a = 1 - 0.5*exp(ur*exp(n2/(r2-n1))*n2);
	}
	ux[j] = a*potential->ihx*pow(ut[j]/n0,ur);
      }
      _jsi = j;
      for (m = j+1; m <= jmax; m++) {
	u[m] += ux[m]*(u[m]/ut[m]);
	if (potential->mps > 2) {
	  potential->ZPS[m] += ux[m]*(potential->ZPS[m]/ut[m]);
	}
	ut[m] += ux[m];
      }
    }
    for (j = jmax+1; j < potential->maxrp; j++) {
      u[j] = u[jmax];
      if (potential->mps > 2) {
	potential->ZPS[j] = potential->ZPS[jmax];
      }
    }
    if (potential->mps > 2) {
      Differential(potential->ZPS, potential->dZPS,
		   0, potential->maxrp-1, potential);
      Differential(potential->dZPS, potential->dZPS2,
		   0, potential->maxrp-1, potential);
    }
    if (_scpot.md == 3) {
      potential->zps = potential->ZPS[potential->maxrp-1];
    }
  } 
  for (m = jmax-1; m > 50; m--) {
    if (fabs(u[m]-u[jmax]) > EPS6) break;
  }
  potential->r_core = m+1;
  return jmax;
}

int SetScreenDensity(AVERAGE_CONFIG *acfg, int iter, int md) {
  ORBITAL *orb;
  double *w, small, large, *w0, *wx, *wx0, wmin;
  int jmax, i, k1, m, mm, i0, i1;
  double a, b, dn0, u, jps0, jps1;
  
  w = _xk;
  wx = _zk;
  for (m = 0; m < potential->maxrp; m++) {
    w[m] = 0.0;
    wx[m] = 0.0;
    if (md == 1) {
      b = potential->EPS[m];
      w[m] = b;
      if (potential->vxf == 2) wx[m] = b;
    }
  }

  if (md == 0) {
    i0 = 0;
    i1 = acfg->n_cores;
    w0 = potential->NPS;
    wmin = _hx_wmin;
    jmax = 0;
  } else {
    i0 = acfg->n_cores;
    i1 = acfg->n_shells;
    w0 = potential->VPS;
    wx0 = potential->VXF;
    wmin = _sc_wmin;
    if (potential->ups > 0) {
      if (potential->mps == 0 && SPMode() > 3) {
	wmin = 10.0;
      }
      jmax = potential->maxrp-1;
    } else {
      jmax = potential->ips;
    }
  }
  b = 0.0;
  for (i = i0; i < i1; i++) {
    if (acfg->nq[i] <= 0) continue;
    k1 = OrbitalExists(acfg->n[i], acfg->kappa[i], 0.0);
    if (k1 < 0) continue;
    orb = GetOrbitalSolved(k1);
    if (orb->wfun == NULL) continue;
    for (m = 0; m <= orb->ilast; m++) {
      large = Large(orb)[m];
      small = Small(orb)[m];
      u = acfg->nq[i]*(large*large + small*small);
      w[m] += u;
      if (md == 1) {
	if (potential->vxf == 1) {
	  wx[m] += u*b;
	} else if (potential->vxf == 2) {
	  wx[m] += u;
	}
      }
    }
    if (jmax < orb->ilast) jmax = orb->ilast;
  }

  if (jmax > 0) {
    b = 0.0;
    mm = 0;
    for (m = 0; m <= jmax; m++) {
      if (w[m] > b) {
	b = w[m];
	mm = m;
      }
    }
    b *= wmin;
    b = Max(b, 1e-20);
    dn0 = 0.0;
    k1 = 0;
    for (m = 0; m <= jmax; m++) {
      a = fabs(w0[m]-w[m])/Max(w[m],b);
      dn0 += a;
      k1++;
    }
    dn0 /= k1; 
    SetOptDPH(md, dn0, iter);
    if (iter > 2) {
      a = optimize_control.sta[md];
      b = 1-a;
      for (m = 0; m < potential->maxrp; m++) {
	w0[m] = b*w0[m] + a*w[m];
      }
      if (md == 1 && potential->vxf == 2) {
	for (m = 0; m < potential->maxrp; m++) {
	  wx0[m] = b*wx0[m] + a*wx[m];
	}
      }
    } else {
      for (m = 0; m < potential->maxrp; m++) {
	w0[m] = w[m];
      }
      if (md == 1 && potential->vxf == 2) {
	for (m = 0; m < potential->maxrp; m++) {
	  wx0[m] = wx[m];
	}
      }
    }
  } else {
    if (md == 0) {
      for (m = 0; m < potential->maxrp; m++) {
	w0[m] = 0.0;
      }
    } else {      
      for (m = 0; m < potential->maxrp; m++) {
	w0[m] = w[m];
	wx0[m] = wx[m];
      }
    }
    SetOptDPH(md, 0.0, iter);
  }
  return jmax;
}

int PotentialHX1(AVERAGE_CONFIG *acfg, int iter, int md) {
  int jmax, jmaxk, m;
  double *ue2, *w2, *u2, *ue1, *ue, *u, *w;
  double jps0, jps1, y, y0, dy, a;

  jps0 = 0.0;
  jps1 = 0.0;
  ue1 = _phase;
  ue = _dphase;
  u = _dphasep;
  w2 = _yk;
  ue2 = _zk;
  u2 = potential->ZPS;
  switch (_scpot.md) {
  case 0:
    for (m = 0; m < potential->maxrp; m++) {
      ue[m] = 0.0;
      ue2[m] = 0.0;
      //u2[m] = 0.0;      
    }
    break;
  case 1:
    for (m = 0; m < potential->maxrp; m++) {
      ue1[m] = 0.0;
      ue[m] = 0.0;
      u[m] = 0.0;
    }
    break;
  case 2:
    for (m = 0; m < potential->maxrp; m++) {
      ue[m] = 0.0;
      u[m] = 0.0;
      ue2[m] = 0.0;
      //u2[m] = 0.0;
    }
    break;
  default:
    for (m = 0; m < potential->maxrp; m++) {
      ue1[m] = 0.0;
      ue[m] = 0.0;
      u[m] = 0.0;
      ue2[m] = 0.0;
      //u2[m] = 0.0;
    }
  }
  
  potential->ahx = potential->hxs*GetHXS(potential);
  if (_scpot.md != 0 && _scpot.md != 2) {
    jmax = SetScreenDensity(acfg, iter, 0);
  } else {
    jmax = _scpot.jmax;
  }
  if (jmax > 0 && _scpot.md != 0) {
    if (potential->ahx) {
      for (m = 0; m < potential->maxrp; m++) {
	ue1[m] = potential->NPS[m];
      }
    }
  }
  jps0 = 0.0;
  if (_scpot.md != 0) {
    jmax = DensityToSZ(potential, potential->NPS, u, ue1, &jps0, 0);
  } 

  if (acfg->n_cores <= 0 && potential->mps < 0 && _scpot.nr == 0) return 0;
  
  w = potential->NPS;
  if (potential->N+potential->nqf > 1 && _csi <= -1) {
    FixAsymptoticPot(jmax, md, iter, w, u, ue1, ue);
  } else {
    for (m = 0; m < potential->maxrp; m++) {
      ue[m] = ue1[m];
    }
  }  
  if (potential->mps >= 0 && potential->mps < 3) {
    SetPotentialPS(potential, iter);
  }

  if (!(potential->mps == 1 ||
	(potential->mps == 0 && potential->tps <= 0))) {
    if (_scpot.md != 1 && _scpot.md != 3) {      
      jmaxk = SetScreenDensity(acfg, iter ,1);
    } else {
      jmaxk = _scpot.jmax;
    }
    if (jmaxk > 0) {
      if (_scpot.md != 1) {
	for (m = 0; m < potential->maxrp; m++) {
	  ue2[m] = potential->VXF[m] + w[m];
	}
      }
      jps1 = 0.0;
      if (_scpot.md != 1) {
	jmaxk = DensityToSZ(potential, potential->VPS, u2, ue2, &jps1, 1);
	if (potential->mps < 3) potential->nqf = u2[jmaxk];
	potential->jps = jps1;
	if (jmaxk > 0) {
	  if (_csi <= -1) {
	    if (iter < _sc_niter+NDPH) {
	      for (m = 0; m < potential->maxrp; m++) {
		_dwork13[m] = ue2[m];
		_dwork14[m] = potential->VPS[m] + w[m];
		_dwork15[m] = u2[m] + u[m];
	      }
	      FixAsymptoticPot(jmaxk, md, iter, _dwork14,
			       _dwork15, _dwork13, ue2);	
	      for (m = 0; m < potential->maxrp; m++) {
		y = (_dwork15[m]-u[m]) - (ue2[m]-ue1[m]);
		y0 = u2[m] - (_dwork13[m]-ue1[m]);
		if (fabs(y0) > 0) {
		  _dwork17[m] = y/y0;
		} else {
		  _dwork17[m] = 1.0;
		}
		u2[m] = y;
	      }
	    } else {
	      for (m = 0; m < potential->maxrp; m++) {
		u2[m] = (u2[m]-(ue2[m]-ue1[m]))*_dwork17[m];
	      }
	    }
	  } else {
	    for (m = 0; m < potential->maxrp; m++) {
	      u2[m] = (u2[m]-(ue2[m]-ue1[m]));
	    }
	  }
	}
	Differential(potential->ZPS, potential->dZPS,
		     0, potential->maxrp-1, potential);
	Differential(potential->dZPS, potential->dZPS2,
		     0, potential->maxrp-1, potential);
	if (_scpot.md == 3) {
	  potential->zps = potential->ZPS[potential->maxrp-1];
	}
      }
    }
  }

  return Max(jmax, jmaxk);
}

void FixAsymptoticPot(int jmax, int md, int iter,
		      double *w, double *u, double *ue1, double *ue) {
  int i, j, k, k1, k2, m, niter;
  double a, b, c, d, d0, d1;
  
  potential->rhx = 0;
  potential->dhx = 0;
  if (jmax > 1 && potential->hxs && potential->ihx < 0 &&
      (_scpot.md < 0 || _scpot.md > 1)) {    
    double ihx = -potential->ihx;
    if (md == 1) {
      if (potential->chx < 1e-3) potential->chx = 1e-3;
      for (m = 0; m < potential->maxrp; m++) {
	if (!w[m]) {
	  break;
	}
	_dwork9[m] = w[m]/(potential->rad[m]*potential->rad[m]);
      }
      m--;
      j = DensityAsymptote(w, &a, &b);
      Differential(_dwork9, _dwork8, 0, m, potential);
      d0 = pow(a, -ONETHIRD);
      d1 = 2*potential->chx*potential->ahx*1e-3;
      c = 2*d0*b;
      k = 0;
      for (i = 0; i <= m; i++) {
	double y3;
	if (potential->rad[i] < potential->atom->rms0) {
	  k = i;
	}
	y3 = pow(_dwork9[i], ONETHIRD);
	_dwork8[i] /= y3*_dwork9[i];	
	ue[i] = ihx*d1*y3*_dwork8[i]*_dwork8[i];
	ue[i] /= (1+3*d1*_dwork8[i]*asinh(_dwork8[i]/c));
	ue[i] *= potential->rad[i];	
	if (i > j) {
	  double z3 = exp(-ONETHIRD*b*potential->rad[i])/d0;
	  double xx = -b/z3;
	  double ux = ihx*d1*z3*xx*xx/(1+3*d1*xx*asinh(xx/c));
	  ux *= potential->rad[i];
	  double wx = potential->rad[i]-potential->rad[j];
	  wx = exp(-b*potential->bhx*wx);
	  if (wx > 1e-3) {
	    ue[i] = pow(ue[i],wx)*pow(ux,1-wx);
	  } else {
	    ue[i] = ux;
	  }
	}
	ue[i] += ue1[i];
      }
      for (; i <= jmax; i++) {
	ue[i] = ue[i-1];
      }
      for (i = 0; i < k; i++) {
	ue[i] = ue[k];
      }
    } else {
      double n0 = potential->N + potential->nqf;
      double n1 = n0 - ihx;
      double n2 = n1 - 0.5*ihx;      
      i = 0;
      j = 0;
      if (n2 > 0) {
	for (m = 0; m <= jmax; m++) {
	  _dwork9[m] = u[m] - ue1[m];
	  if (_dwork9[m] > n2 && i == 0) {
	    i = m;
	  }
	  if (_dwork9[m] > n1 && j == 0) {
	    j = m;
	  }
	  if (j > 0 && m - j > 6) break;
	}
      }
      if (i > 0 && j > i) {
	k1 = i-5;
	if (k1 < 0) k1 = 0;
	for (k2 = i; k2 < m; k2++) {
	  if (_dwork9[k2+1] <= _dwork9[k2]) break;
	  if (k2-i > 5) break;
	}
	k = k2 - k1 + 1;
	d0 = potential->rho[k1];
	d1 = potential->rho[k2];
	d = 0.5*(d0+d1);
	niter = 0;
	while (d1-d0 > EPS5*fabs(d)) {
	  niter++;
	  d = 0.5*(d0+d1);
	  UVIP3P(3, k, potential->rho+k1, _dwork9+k1, 1, &d, &a);
	  if (niter > 100) {
	    printf("d01a: %d %d %d %g %g %g %g %g\n", niter,k1,k,d0,d1,d,a,n2);
	    Abort(1);
	  }
	  if (a < n2) {
	    d0 = d;
	  } else if (a > n2) {
	    d1 = d;
	  } else {
	    break;
	  }
	}
	a = GetRFromRho(d, potential->ar, potential->br, potential->qr,
			potential->rad[i]);
	potential->rhx = a;    
	k1 = j-5;
	if (k1 < 0) k1 = 0;
	for (k2 = j; k2 < m; k2++) {
	  if (_dwork9[k2+1] <= _dwork9[k2]) break;
	  if (k2-j > 5) break;
	}
	k = k2 - k1 + 1;
	d0 = potential->rho[k1];
	d1 = potential->rho[k2];
	d = 0.5*(d0+d1);
	niter = 0;
	while (d1-d0 > EPS5*fabs(d)) {
	  niter++;
	  d = 0.5*(d0+d1);
	  UVIP3P(3, k, potential->rho+k1, _dwork9+k1, 1, &d, &a);
	  if (niter > 100) {
	    printf("d01b: %d %d %d %g %g %g %g %g\n", niter,k1,k,d0,d1,d,a,n2);
	    Abort(1);
	  }
	  if (a < n1) {
	    d0 = d;
	  } else if (a > n1) {
	    d1 = d;
	  } else {
	    break;
	  }
	}
	a = GetRFromRho(d, potential->ar, potential->br, potential->qr,
			potential->rad[j]);
	potential->dhx = a;        
	for (i = j; i > 0; i--) {
	  if (ue1[i-1] < ue1[i]) {
	    break;
	  }
	}
	c = potential->rad[i];
	b = potential->rho[i];
	k = 0;
	for (j = i; j < jmax; j++, k++) {
	  if (ue1[j+1] >= ue1[j]) break;
	  if (k > 5) break;
	}
	k = 0;
	for (; i > 0; i--, k++) {
	  if (ue1[i-1] >= ue1[i]) break;
	  if (k > 5) break;
	}
	Differential(ue1, _dwork9, i, j, potential);
	d0 = potential->rho[i];
	d1 = potential->rho[j];
	d = 0.5*(d0+d1);
	k = j-i+1;
	while (d1-d0 > EPS5*fabs(d)) {
	  d = 0.5*(d0+d1);
	  UVIP3P(3, k, potential->rho+i, _dwork9+i, 1, &d, &a);
	  if (a > 0) {
	    d0 = d;
	  } else if (a < 0) {
	    d1 = d;
	  } else {
	    break;
	  }
	}
	a = GetRFromRho(d, potential->ar, potential->br, potential->qr, c);
	if (a < potential->rhx) {
	  potential->rhx = a;
	}
	potential->dhx = (potential->dhx - potential->rhx)*potential->bhx;
	c = 1.0 - n1/n0;
	for (m = 0; m <= jmax; m++) {
	  d = (potential->rad[m]-potential->rhx)/potential->dhx;
	  d = 1.0/(1.0 + exp(-d));
	  d0 = u[m]*c;
	  d1 = ue1[m];
	  d0 = Max(d0, ue1[m]);
	  d0 = Min(d0, u[m]);
	  d1 = Min(d1, u[m]);
	  ue[m] = d0*d + d1*(1-d);
	  if (d > 0.5 && u[m]-ue[m] > n1) {
	    ue[m] = u[m]-n1;
	  }
	}
      } else {
	if (n2 <= 0) {
	  for (m = 0; m <= jmax; m++) {
	    ue[m] = u[m];
	  }
	} else {
	  for (m = 0; m <= jmax; m++) {
	    ue[m] = ue1[m];
	  }
	}
      }
    }
  } else {
    for (m = 0; m <= jmax; m++) {
      ue[m] = ue1[m];
    }
  }
  if (jmax < 0) {
    for (m = 0; m < potential->maxrp; m++) {
      ue[m] = 0.0;
    }
  } else {
    for (m = jmax+1; m < potential->maxrp; m++) {
      u[m] = u[jmax];
      ue[m] = ue[jmax];
    }
  }
}

void SetPotential(AVERAGE_CONFIG *acfg, int iter) {
  int jmax, i, j;
  double *u, a, b, c, r;
  u = potential->U;
  jmax = PotentialHX(acfg, u, iter);
  if (jmax > 0) {
    AdjustScreeningParams(u);
    SetPotentialVc(potential);
    for (j = 0; j < potential->maxrp; j++) {
      a = u[j] - potential->Z[j];
      b = potential->Vc[j]*potential->rad[j];
      u[j] = a - b;
      u[j] /= potential->rad[j];
    }
    SetPotentialU(potential, 0, NULL);
    SetPotentialVT(potential);
  } else {
    if (potential->N < 1.0+EPS3) {
      SetPotentialVc(potential);
      SetPotentialU(potential, -1, NULL);
      SetPotentialVT(potential);
      if (potential->mps < 0 && _scpot.md != 1 && _scpot.md != 3) return;
    }
    r = potential->atom->atomic_number;
    b = potential->N1/potential->N;
    for (i = 0; i < acfg->n_cores; i++) {
      a = acfg->nq[i];
      c = acfg->n[i];
      c = r/(c*c);
      for (j = 0; j < potential->maxrp; j++) {
	if (potential->rad[j] < 0.1*potential->atom->rn) {
	  u[j] = 0.0;
	} else {
	  u[j] += a*b*(1.0 - exp(-c*potential->rad[j]));
	}
      }
    }
    AdjustScreeningParams(u);
    SetPotentialVc(potential);
    for (j = 0; j < potential->maxrp; j++) {
      a = u[j] - potential->Z[j];
      b = potential->Vc[j]*potential->rad[j];
      u[j] = a - b;
      u[j] /= potential->rad[j];
    }
    
    SetPotentialU(potential, 0, NULL);
    SetPotentialVT(potential);
  }
}

void OrbitalStats(char *s, int nmax, int kmax) {
  int n, k, km, j, ka, idx, nm;
  ORBITAL *orb;
  FILE *f;
  char c, sk[8];
  double r1, r2, ri1, ri2;

  f = fopen(s, "w");
  if (f == NULL) {
    printf("cannot open file %s\n", s);
    return;
  }
  if (nmax <= 0) {
    printf("nmax <= 0 in OrbitalStats\n");
    return;
  }
  for (n = 1; n <= nmax; n++) {
    km = n-1;
    if (kmax >= 0 && km > kmax) {
      km = kmax;
    }
    for (k = 0; k <= km; k++) {
      for (j = -1; j <= 1; j += 2) {
	if (k == 0 && j < 0) continue;
	if (j < 0) c = '-';
	else c = '+';
	SpecSymbol(sk, k);
	ka = GetKappaFromJL(2*k+j, 2*k);
	nm = abs(GetOrbNMax(ka, 0));
	if (nm && n > nm) continue;
	idx = OrbitalIndex(n, ka, 0);
	orb = GetOrbitalSolved(idx);
	ri1 = RadialMoments(-1, idx, idx);
	ri2 = RadialMoments(-2, idx, idx);
	r1 = RadialMoments(1, idx, idx);
	r2 = RadialMoments(2, idx, idx);
	fprintf(f, "%3.0f %3.0f %3d %3d %3d %3d%s%c %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E\n",
		potential->atom->atomic_number,
		potential->N, n, k, ka, n, sk, c,
		orb->energy, orb->dn, ri2, ri1, r1, r2);
	
      }
    }
  }
  fclose(f);
}

int GetPotential(char *s, int m) {
  AVERAGE_CONFIG *acfg;
  ORBITAL *orb1, *orb2;
  double large1, small1, large2, small2;
  int norbs, jmax, kmin, kmax;  
  FILE *f;
  int i, j;
  double rb, rb0, rb1, rc;

  /* get the average configuration for the groups */
  acfg = &(average_config);

  f = fopen(s, "w");
  if (!f) return -1;
  
  fprintf(f, "# Lambda = %15.8E\n", potential->lambda);
  fprintf(f, "#      A = %15.8E\n", potential->a);
  fprintf(f, "#     ar = %15.8E\n", potential->ar);
  fprintf(f, "#     br = %15.8E\n", potential->br);
  fprintf(f, "#     qr = %15.8E\n", potential->qr);
  fprintf(f, "#    irc = %d\n", potential->r_core);
  rc = potential->r_core > 0?potential->rad[potential->r_core]:0;
  fprintf(f, "#     rc = %15.8E\n", rc);
  rb = potential->ib>0?potential->rad[potential->ib]:0;
  rb0 = potential->ib0>0?potential->rad[potential->ib0]:0;
  rb1 = potential->ib1>0?potential->rad[potential->ib1]:0;
  fprintf(f, "#     ib = %d\n", potential->ib);
  fprintf(f, "#    ib0 = %d\n", potential->ib0);
  fprintf(f, "#    ib1 = %d\n", potential->ib1);
  fprintf(f, "#     rb = %15.8E\n", rb);
  fprintf(f, "#    rb0 = %15.8E\n", rb0);
  fprintf(f, "#    rb1 = %15.8E\n", rb1);
  fprintf(f, "#   rmin = %15.8E\n", potential->rad[0]);
  fprintf(f, "#   rmax = %15.8E\n", potential->rad[potential->maxrp-1]);
  fprintf(f, "#  ratio = %15.8E\n", potential->ratio);
  fprintf(f, "#  asymp = %15.8E\n", potential->asymp);
  fprintf(f, "#    bqp = %15.8E\n", potential->bqp);
  fprintf(f, "#     nb = %d\n", potential->nb);
  fprintf(f, "#  miter = %d\n", potential->miter);
  fprintf(f, "#   mode = %d\n", potential->mode);
  fprintf(f, "#    HXS = %15.8E\n", potential->hxs);
  fprintf(f, "#    AHX = %15.8E\n", potential->ahx);
  fprintf(f, "#    IHX = %15.8E\n", potential->ihx);
  fprintf(f, "#    RHX = %15.8E\n", potential->rhx);
  fprintf(f, "#    DHX = %15.8E\n", potential->dhx);
  fprintf(f, "#    BHX = %15.8E\n", potential->bhx);
  fprintf(f, "#    CHX = %15.8E\n", potential->chx);
  fprintf(f, "#    HX0 = %15.8E\n", potential->hx0);
  fprintf(f, "#    HX1 = %15.8E\n", potential->hx1);
  fprintf(f, "#      Z = %15.8E\n", potential->atom->atomic_number);
  fprintf(f, "#      N = %15.8E\n", potential->N);
  fprintf(f, "#     NC = %15.8E\n", potential->NC);  
  fprintf(f, "#    mps = %d\n", potential->mps);
  fprintf(f, "#    kps = %d\n", potential->kps);
  fprintf(f, "#    vxf = %d\n", potential->vxf);
  fprintf(f, "#    vxm = %d\n", potential->vxm);
  fprintf(f, "#    mhx = %d\n", _mhx);
  fprintf(f, "#    xhx = %15.8E\n", _xhx);
  fprintf(f, "#    csi = %15.8E\n", _csi);
  fprintf(f, "#    zps = %15.8E\n", potential->zps);
  fprintf(f, "#    nps = %15.8E\n", potential->nps);
  fprintf(f, "#    tps = %15.8E\n", potential->tps);
  fprintf(f, "#    ups = %15.8E\n", potential->ups);
  fprintf(f, "#    rps = %15.8E\n", potential->rps);
  fprintf(f, "#    cps = %15.8E\n", potential->cps);  
  fprintf(f, "#    dps = %15.8E\n", potential->dps);
  fprintf(f, "#    aps = %15.8E\n", potential->aps);
  fprintf(f, "#    bps = %15.8E\n", potential->bps);
  fprintf(f, "#    nbt = %15.8E\n", potential->nbt);
  fprintf(f, "#    nbs = %15.8E\n", potential->nbs);
  fprintf(f, "#    nqf = %15.8E\n", potential->nqf);
  fprintf(f, "#    efm = %15.8E\n", potential->efm);
  fprintf(f, "#    eth = %15.8E\n", potential->eth);
  fprintf(f, "#    ewd = %15.8E\n", potential->ewd);
  fprintf(f, "#    ewf = %15.8E\n", SCEWF());
  fprintf(f, "#    ewm = %d\n", SCEWM());
  fprintf(f, "#    fps = %15.8E\n", potential->fps);
  fprintf(f, "#    xps = %15.8E\n", potential->xps);
  fprintf(f, "#    jps = %15.8E\n", potential->jps);
  fprintf(f, "#    gps = %15.8E\n", potential->gps);
  fprintf(f, "#    qps = %15.8E\n", potential->qps);
  fprintf(f, "#    sf0 = %15.8E\n", potential->sf0);
  fprintf(f, "#    sf1 = %15.8E\n", potential->sf1);
  fprintf(f, "#    ips = %d\n", potential->ips);
  fprintf(f, "#    sps = %d\n", potential->sps);
  fprintf(f, "#  sturm = %15.8E\n", potential->sturm_idx);
  fprintf(f, "#   nmax = %d\n", potential->nmax);
  fprintf(f, "#  maxrp = %d\n", potential->maxrp);
  PrintLepton(f);
  fprintf(f, "# Mean configuration: %d %d\n", acfg->n_shells, acfg->n_cores);
  double nqt = 0.0;
  double nqb = 0.0;
  double nqc = 0.0;
  double x;
  char wvf[1024];
  for (i = 0; i < acfg->n_shells; i++) {
    j = OrbitalIndex(acfg->n[i], acfg->kappa[i], 0);
    orb1 = GetOrbitalSolved(j);
    x = 1.0;
    if (i < acfg->n_cores) nqc += acfg->nq[i];
    else x = BoundFactor(orb1->energy, potential->eth,
			 potential->ewd, potential->mps);
    if (x < 1e-50) x = 0.0;
    nqb += acfg->nq[i]*x;
    nqt += acfg->nq[i];
    fprintf(f, "# %4d %4d %4d %12.5E %12.5E %12.5E %12.5E %12.5E %15.8E\n",
	    i, acfg->n[i], acfg->kappa[i], acfg->nq[i],
	    nqc, nqb, nqt, x, orb1->energy);
    if (m) {
      if (m < 0 || abs(orb1->n) <= m) {
	if (orb1->kappa > 0) {
	  sprintf(wvf, "wv%03dk%03dm.txt", abs(orb1->n), orb1->kappa);
	} else {
	  sprintf(wvf, "wv%03dk%03dp.txt", abs(orb1->n), -orb1->kappa-1);
	}
	WaveFuncTableOrb(wvf, orb1);
      }
    }
  }
  fprintf(f, "\n\n");
  double *vxf = potential->VXF;
  double *icf = potential->ICF;
  for (i = 0; i < potential->maxrp; i++) {
    fprintf(f, "%5d %14.8E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E %14.8E\n",
	    i, potential->rad[i], potential->Z[i],
	    potential->Z[i]-GetAtomicEffectiveZ(potential->rad[i]),
	    potential->Vc[i]*potential->rad[i],
	    potential->U[i]*potential->rad[i],
	    potential->ZSE[0][i],
	    potential->ZSE[1][i],
	    potential->ZSE[2][i],
	    potential->ZSE[3][i],
	    potential->ZSE[4][i],
	    potential->ZVP[i],
	    potential->NPS[i],
	    potential->EPS[i],
	    potential->VPS[i],
	    vxf[i],
	    icf[i],
	    potential->ZPS[i]);
  }    
  fclose(f);    
  return 0;
}

double GetResidualZ(void) {
  double z;
  z = potential->atom->atomic_number;
  if (potential->N > 0) z -= potential->N1;
  return z;
}

double GetRMax(void) {
  return potential->rad[potential->maxrp-10];
}

int SetAverageConfig(int nshells, int *n, int *kappa, double *nq) {
  int i, j;
  if (average_config.n_allocs > 0) {
    free(average_config.kappa);
    free(average_config.nq);
    free(average_config.n);
    free(average_config.e);
    if (average_config.ng > 0) {
      free(average_config.kg);
      free(average_config.weight);
    }
  }
  if (nshells < 0) {
    return -1;
  }
  if (nshells > 0) {
    average_config.kappa = (int *) malloc(sizeof(int)*nshells);
    average_config.nq = (double *) malloc(sizeof(double)*nshells);
    average_config.n = (int *) malloc(sizeof(int)*nshells);
    average_config.e = (double *) malloc(sizeof(double)*nshells);
  }
  j = 0;
  for (i = 0; i < nshells; i++) {
    if (nq[i] <= 0.0) continue;
    average_config.n[j] = n[i];
    average_config.kappa[j] = kappa[i];
    average_config.nq[j] = nq[i];
    average_config.e[j] = 0.0;
    j++;
  }
  average_config.n_shells = j;
  average_config.n_cfgs = 1;
  if (j == 0 && nshells > 0) {
    free(average_config.kappa);
    free(average_config.nq);
    free(average_config.n);
    free(average_config.e);
  }
  average_config.n_cores = j;
  average_config.n_allocs = nshells;
  return 0;
}

void SetScreenConfig(int iter) {
  int k, k2, ka, n0, n1, n, i, j, m, nf, nfk;
  AVERAGE_CONFIG *a = &average_config;
  ORBITAL *orb;
  int na = 2000;
  double e0, ef0, tps, etf;
  double *nq, *nqc, *et;
  int *np, *kp, it, iti;
  double nb, nbt, nqt, nb0, nb1, u, du, u0, u1, x, nqf, nqf0, eth;

  if (potential->mps < 0 || potential->nbt < 0) return;
  if (potential->mps >= 3 && potential->nbt < 1e-10) {
    potential->nqf = 0.0;
    potential->nbs = 0.0;
    return;
  }
  eth = 0.0;
  if (potential->rps > 0 && fabs(_free_threshold) > EPS10) {
    eth = (1.0/potential->rps)*_free_threshold;
  }
  potential->eth = eth;
  nbt = potential->nbt;
  k = 0;
  n = 0;
  m = a->n_cores;
  nf = 0;
  ef0 = potential->eth;
  nqf0 = potential->nqf;
  tps = potential->tps;
  if (SCEWM() <= 0) {
    etf = -1E31;
  } else {
    etf = eth;
  }
  if (SCEWM() < 0) {
    a->n_shells = m;
    for (m = 0; m < a->n_shells; m++) {
      orb = GetOrbitalSolved(OrbitalIndex(a->n[m], a->kappa[m], 0));
      a->e[m] = orb->energy/tps;
    }
    potential->efm = 0.0;
    e0 = potential->efm;    
  } else if (iter < _sc_niter) {
    if (potential->mps >= 3) {
      e0 = potential->ewd*_psemax;
    } else {
      e0 = 0.0;
    }
    it = 0;
    int mb = 0;
    int nqm = 0;
    nqt = 0.0;
    for (i = 0; i < a->n_cores; i++) {
      nqt += a->nq[i];
    }
    while (1) {
      it++;
      if (it > 10000) {
	printf("sc loop fail: %d %g\n", it, e0);
	Abort(1);
      }
      k2 = k*2;
      int donem = 0, donep = 0;
      for (j = -1; j <= 1; j += 2) {
	if (k == 0 && j < 0) continue;
	nqm = k2+j+1;
	nqt += nqm;
	ka = GetKappaFromJL(k2+j, k2);
	nfk = 0;
	n0 = 1000000000;
	n1 = 0;
	for (i = 0; i < a->n_cores; i++) {
	  if (a->nq[i] < nqm) continue;
	  if (a->kappa[i] != ka) continue;
	  if (a->n[i] > n1) n1 = a->n[i];
	  if (a->n[i] < n0) n0 = a->n[i];
	}
	n = k+1;
	if (n < _psnmin) n = _psnmin;
	int done = 0;
	iti = 0;
	while (1) {
	  iti++;
	  if (iti > 10000) {
	    printf("sc iloop fail: %d %d %d %d %d\n", it, iti, n, n0, n1);
	    Abort(1);
	  }
	  if (n >= n0 && n <= n1) {
	    for (i = 0; i < a->n_cores; i++) {
	      if (a->nq[i] < nqm) continue;
	      if (a->kappa[i] != ka) continue;
	      if (a->n[i] == n) break;
	    }
	    if (i < a->n_cores) {
	      n++;
	      continue;
	    }
	  }
	  orb = GetOrbitalSolved(OrbitalIndex(n, ka, 0));
	  if (potential->mps >= 3 && _psemax > 0 && iter > 1) {
	    if (k == 0 && orb->energy > eth) {
	      if (_psnfmax > 0 && nf > _psnfmax) {
		e0 = 0.5*(ef0+orb->energy);
		mb = 1;
		break;
	      }
	      ef0 = orb->energy;
	      if (nf <= _psnfmin) {
		e0 = ef0 + _psemax*potential->ewd;
	      }
	    }
	  } else {	    
	    if (orb->energy > eth) done = 1;
	    if (iter == 0) {
	      if (n > 5) {
		done = 1;
	      } else if ((_psnfmin < 0 || nf > _psnfmin || nf > _psnfmax) &&
			 (orb->energy > e0 || nqt > nbt+1)) {
		done = 1;
	      }
	    }
	  }
	  if ((_psnfmin < 0 || nfk >= _psnfmin) &&
	      orb->energy > eth+_psemax*potential->ewd) {
	    mb = 2;
	    break;
	  }
	  if (_psnmax > 0 && n > _psnmax) {
	    mb = 4;
	    break;
	  }
	  if (m == a->n_allocs) {
	    if (a->n_allocs == 0) {
	      a->n_allocs += na;	  
	      a->n = malloc(sizeof(int)*a->n_allocs);
	      a->kappa = malloc(sizeof(int)*a->n_allocs);
	      a->nq = malloc(sizeof(double)*a->n_allocs);
	      a->e =malloc(sizeof(double)*a->n_allocs);
	    } else {
	      a->n_allocs += na;	  
	      a->n = realloc(a->n, sizeof(int)*a->n_allocs);
	      a->kappa = realloc(a->kappa, sizeof(int)*a->n_allocs);
	      a->nq = realloc(a->nq, sizeof(double)*a->n_allocs);
	      a->e = realloc(a->e, sizeof(double)*a->n_allocs);
	    }
	  }
	  a->n[m] = n;
	  a->kappa[m] = ka;
	  a->e[m] = orb->energy/tps;
	  mb = 0;
	  m++;
	  if (k == 0 && orb->energy > eth) nf++;
	  if (orb->energy > eth) nfk++;
	  if (done) break;
	  n++;
	}
	if (n <= k+1+_psnfmin) {
	  if (j < 0) donem = 1;
	  else donep = 1;
	}
      }
      if (donem && donep) break;
      k++;
      if (_pskmax >= 0 && k > _pskmax) break;
    }
    if (mb) m--;
    a->n_shells = m;
    potential->efm = e0;
  } else {
    for (m = 0; m < a->n_shells; m++) {
      orb = GetOrbitalSolved(OrbitalIndex(a->n[m], a->kappa[m], 0));
      a->e[m] = orb->energy/tps;
    }
    e0 = potential->efm;
  }
  if (potential->mps <= 3) e0 = -1E31;
  nqf = 0;
  u = potential->bps;
  int ns = a->n_shells - a->n_cores;
  if (ns <= 0 && e0 < eth) return;
  if (ns > 0) {
    nqc = malloc(sizeof(double)*ns);
  } else {
    nqc = NULL;
  }
  np = a->n + a->n_cores;
  kp = a->kappa + a->n_cores;
  nq = a->nq + a->n_cores;
  et = a->e + a->n_cores;
  for (i = 0; i < ns; i++) {
    nqc[i] = 0.0;
    for (m = 0; m < a->n_cores; m++) {
      if (np[i] == a->n[m] && kp[i] == a->kappa[m]) {
	nqc[i] = a->nq[m];
	break;
      }
    }
  }
  
  if (nbt < 1e-10) {
    u = potential->aps;
    nb = NBoundAA(ns, np, kp, nq, et, nqc, u, tps, etf, &nqf);
  } else {
    if (ns > 0) {
      double *amu, *anb;
      int ns2 = ns+2;
      amu = malloc(sizeof(double)*ns2);
      anb = malloc(sizeof(double)*ns2);
      int *ik = malloc(sizeof(int)*ns2);
      ArgSort(ns, et, ik);
      u1 = fabs(et[ik[0]]);
      amu[0] = et[ik[0]]-2*u1;
      u0 = fabs(et[ik[ns-1]]);
      u0 = Max(5.0, u0);
      u0 = Min(u0, u1);
      amu[ns2-1] = et[ik[ns-1]]+u0;
      for (i = 0; i < ns; i++) {
	if (i < ns-1) amu[i+1] = et[ik[i]]*0.75+et[ik[i+1]]*0.25;
	else amu[i+1] = et[ik[i]]*0.75+amu[i+2]*0.25;
      }
      k = -1;
      for (i = 0; i < ns2; i++) {
	anb[i] = NBoundAA(ns, np, kp, nq, et, nqc, amu[i], tps, etf, &nqf);
	if (k < 0 && anb[i] > nbt) {
	  k = i;
	  break;
	}
      }
      if (k < 0) {
	k = ns;
      }
      u = amu[k];
      du = 1.0;
      for (i = k-1; i>=0; i--) {
	if (anb[k]-anb[i] > 0.1) {
	  du = nbt*(amu[k]-amu[i])/(anb[k]-anb[i]);
	  break;
	}
      }
      du = Max(1.0, du);
      free(amu);
      free(anb);
      free(ik);
    } else {
      x = potential->nps;
      u = FermiDegeneracy(0.1*x, potential->tps, &nqf);
      du = FermiDegeneracy(potential->atom->atomic_number*x,
			   potential->tps, &nqf);
      du = fabs(du-u);
      du = Max(1.0, du);
    }
    nb = NBoundAA(ns, np, kp, nq, et, nqc, u, tps, etf, &nqf);
    it = 0;
    u0 = u;
    u1 = u;
    nb0 = nb;
    nb1 = nb;
    if (nb > nbt) {
      while (1) {
	while (nb > nbt) {
	  u1 = u;
	  nb1 = nb;
	  u -= du;
	  nb = NBoundAA(ns, np, kp, nq, et, nqc, u, tps, etf, &nqf);	
	  it++;
	  if (it > optimize_control.maxiter-5) {
	    printf("maxiter reached in sc0: %d %d %g %g %g %g %g %g %g\n", it, ns, nbt, nb, u, nb1, u1, du, nqf);
	    if (it > optimize_control.maxiter) {
	      Abort(1);
	    }
	  }
	  x = nb1-nb;
	  x = (nb1-nbt)/Max(1e-3,x);
	  if (x > 1) {
	    x = pow(x, 0.35);	    
	    x = Min(5.0, x);
	    du *= x;
	  }
	}
	if (nb1-nb < 1.5) break;
	u = u1;
	nb = nb1;
	du *= 0.25;
      }
      u0 = u;
      nb0 = nb;
    } else if (nb < nbt) {
      while (1) {
	while (nb < nbt) {
	  u0 = u;
	  nb0 = nb;
	  u += du;
	  nb = NBoundAA(ns, np, kp, nq, et, nqc, u, tps, etf, &nqf);
	  it++;
	  if (it > optimize_control.maxiter-5) {	      
	    printf("maxiter reached in sc1: %d %d %g %g %g %g %g %g %g\n", it, ns, nbt, nb, u, nb0, u0, du, nqf);
	    if (it > optimize_control.maxiter) {
	      Abort(1);
	    }
	  }
	  x = nb-nb0;
	  x = (nbt-nb)/Max(1e-3,x);
	  if (x > 1) {
	    x = pow(x, 0.35);
	    x = Min(5.0, x);	  
	    du *= x;
	  }
	}
	if (nb-nb0 < 1.5) break;
	u = u0;
	nb = nb0;
	du *= 0.25;
      }
      u1 = u;
      nb1 = nb;
    }
    int nx = 7, iu;
    double ug[7], ng[7];
    double fu;
    if (e0 >= eth && fabs(nb1-nb0) > 1e-6) {
      fu = (u1-u0)/(nx-1);
      ug[0] = u0;
      for (iu = 1; iu < nx; iu++) {
	ug[iu] = ug[iu-1] + fu;
      }
      for (iu = 0; iu < nx; iu++) {
	ng[iu] = FreeElectronDensity(potential, etf, ug[iu], 0.0, 2, 0);
      }
    }
    it = 0;
    while (fabs(nb1-nb0) > 1e-6) {
      x = (nb1-nbt)/(nbt-nb0);
      if (x < 0.25) x = 0.25;
      if (x > 0.75) x = 0.75;
      u = x*u0 + (1-x)*u1;
      nb = NBoundAA(ns, np, kp, nq, et, nqc, u, tps, 2E31, NULL);
      if (e0 >= eth) {
	UVIP3P(3, nx, ug, ng, 1, &u, &nqf);
      }
      nb += nqf;
      if (fabs(nb-nbt) < 1e-6) break;
      if (nb < nbt) {
	u0 = u;
	nb0 = nb;
      } else {
	u1 = u;
	nb1 = nb;
      }
      it++;
      if (it > optimize_control.maxiter) {
	printf("maxiter reached in sc2: %d %g %g %g %g %g %g %g\n", it, nb, nbt, nqf, u, u0, u1, nb1-nb0);
	Abort(1);
      }
    }
    if (e0 >= eth) {
      nb = NBoundAA(ns, np, kp, nq, et, nqc, u, tps, etf, &nqf);
    }
  }
  potential->bps = u;
  potential->nbs = 0.0;
  for (i = 0; i < ns; i++) {
    potential->nbs += nq[i];
  }
  if (e0 >= eth) {
    potential->nqf = nqf;
  }
  SetPotentialN();
  if (optimize_control.iset == 0) {
    double z0 = potential->atom->atomic_number;
    double zr = z0-potential->N;    
    optimize_control.stabilizer = 0.25 + 0.5*(zr/z0);
  }
  if (ns > 0) {
    free(nqc);
  }
  if (_sc_print) {
    printf("sc: %3d %3d %4d %12.5E %12.5E %8.4f %7.3f %7.3f %7.3f %10.4E %7.2E %7.2E %7.2E\n",
	   iter, nf, ns, potential->aps, potential->bps,
	   potential->nbs, potential->nbt, nb, nqf, potential->ewd,
	   optimize_control.dph[0][NDPH-1],
	   optimize_control.dph[1][NDPH-1],
	   optimize_control.sta[1]);
  }
}

void FreezeOrbital(char *s, int m) {
  char s0[1024], *p;
  int i, j, k, n, nc;
  CONFIG *cfg;
  ORBITAL *orb;

  if (m >= 0) _orthogonalize_mode = m;
  strncpy(s0, s, 1023);
  s = s0;
  while (*s == ' ') s++;
  if (*s == '\0') return;
  n = StrSplit(s, ' ');
  p = s;
  for (i = 0; i < n; i++) {
    while (*p == ' ') p++;
    nc = GetConfigFromString(&cfg, p);
    for (j = 0; j < nc; j++) {
      if (cfg[j].n_shells != 1) {
	printf("incorrect freeze orbital format: %d %s\n", i, p);
	continue;
      }
      k = OrbitalExists((cfg[j].shells)[0].n, (cfg[j].shells)[0].kappa, 0);
      if (k < 0) continue;
      orb = GetOrbital(k);
      if (orb->isol == 1) {
	orb->isol = 2;
	potential->nfrozen++;
      }
    }
    if (nc > 0) free(cfg);
    while (*p) p++;
    p++;
  }
}

int OptimizeILoop(AVERAGE_CONFIG *acfg, int iter, int miter,
		  double emin, double emax, int sc) {
  double tol, atol, tol0, atol0, tol1, a, b, ahx, hxs0, ihx0;
  double *p, *q, wp, wq, w;
  ORBITAL orb_old, *orb;
  int i, k, m, no_old, *freeze;

  no_old = 0;
  tol = 1.0;
  atol = 1e1;
  tol0 = optimize_control.tolerance*ENERELERR;
  tol1 = optimize_control.tolerance*ENERELERR1;
  atol0 = optimize_control.tolerance*ENEABSERR;
  ahx = 1.0;
  ihx0 = potential->ihx;
  hxs0 = potential->hxs;
  if (_scpot.md == 0 || _scpot.md == 1 ||
      (fabs(hxs0) < 1e-5 && fabs(ihx0) < 1e-5)) ahx = 0.0;
  if (iter == 0) SetOptDPH(-1, 0.0, iter);
  double az0 = -1.0;
  double af0 = -1.0;
  double au0 = -1.0;
  double sta = 1.0;  
  int ierr, muconv;
  muconv = 0;
  potential->miter = -1;
  if (_mhx > 0) {
    potential->ihx = 0.0;
    a = potential->atom->atomic_number-potential->N;
    if (a < _ihxmin) potential->ihx = -_ihxmin;
  }
  freeze = malloc(sizeof(int)*acfg->n_shells);
  for (i = 0; i < acfg->n_shells; i++) {
    freeze[i] = 0;
  }
  while (1) {
    if (((tol > tol0*sta || atol > atol0*sta) && (tol > tol1*sta)) ||
	iter <= 1+NDPH || ahx > 1e-5 || muconv == 0) {
      potential->miter = -1;
    } else {
      potential->miter = iter;
      if (_mhx == 1) {
	potential->ihx = ihx0;
      }
    }
    if ((_scpot.md < 0 || _scpot.md > 1) &&
	(fabs(hxs0) > 1e-5 || fabs(ihx0) > 1e-5))  {
      ahx = exp(-iter*0.75);
      if (ahx < 1e-4) ahx = 0.0;
      potential->hxs = hxs0*(1-ahx);
    }
    SetPotential(acfg, iter);
    if (potential->mps == 0 && potential->ups > 0 && SPMode() > 3) {
      a = optimize_control.dph[0][NDPH-1];
    } else {
      a = Max(optimize_control.dph[0][NDPH-1], optimize_control.dph[1][NDPH-1]);
    }
    tol = 0.0;
    atol = 0.0;
    for (i = 0; i < acfg->n_shells; i++) {
      if (freeze[i]) continue;
      k = OrbitalExists(acfg->n[i], acfg->kappa[i], 0.0);
      if (k < 0) {
	orb_old.energy = 0.0;
	orb = GetNewOrbital(acfg->n[i], acfg->kappa[i], 1.0);
	orb->energy = 1.0;
	no_old = 1;	
      } else {	
	orb = GetOrbital(k);
	if (orb->isol == 0 || orb->wfun == NULL) {
	  orb_old.energy = 0.0;
	  orb->energy = 1.0;
	  orb->kappa = acfg->kappa[i];
	  orb->n = acfg->n[i];
	  no_old = 1;	
	} else if (orb->isol == 1) {
	  orb_old.energy = orb->energy;
	  if (_jsi > 0 && iter > NDPH+1 && ahx < 1e-5) {
	    p = Large(orb);
	    q = Small(orb);
	    m = Min(_jsi, orb->ilast);
	    wp = InnerProduct(0, m, p, p, potential);
	    wq = InnerProduct(0, m, q, q, potential);
	    w = wp+wq;
	    if (w < _wsi) {
	      b = fabs(orb_old.energy - orb->energy);
	      wp = fabs(orb_old.energy);
	      wq = fabs(orb->energy);
	      wq = b/Max(wp, wq);
	      wp = sta/w;
	      wp = Min(1.0, wp);
	      if ((b < atol0*wp && wq < tol0*wp) || wq < tol1*wp) {
		freeze[i] = 1;
	      }
	      //printf("wf: %d %d %d %d %d %g %g %g %g %d\n", iter, orb->n, orb->kappa, orb->ilast, _jsi, potential->rad[_jsi], w, orb->energy, orb_old.energy, freeze);
	    }
	  }
	  if (!freeze[i] && orb->energy >= emin && orb->energy < emax) {
	    free(orb->wfun);
	    orb->wfun = NULL;
	    orb->isol = 0;
	  } else {
	    freeze[i] = 1;
	  }	    
	  no_old = 0;
	} else {
	  continue;
	}
      }

      if (!freeze[i]) {
	ierr = SolveDirac(orb);
	if (ierr < 0) {
	  return -1;
	}
      }
      if (no_old) { 
	tol = 1.0;
	atol = 1e1;
	continue;
      }
      if (!freeze[i]) {
	wp = fabs(orb_old.energy);
	wq = fabs(orb->energy);
	w = Max(wp, wq);
	b = fabs(orb_old.energy - orb->energy);
	if (atol < b) atol = b;
	b /= w;
	if (tol < b) tol = b;
      }
    }
    if (tol < a) tol = a;
    if (optimize_control.iprint) {
      printf("optimize loop: %4d %13.5E %13.5E %13.5E %13.5E %13.5E %13.5E %13.5E %10.3E %10.3E %10.3E %10.3E\n",
	     iter, tol, atol, a, tol0, tol1, atol0, ahx,
	     optimize_control.dph[0][NDPH-1],
	     optimize_control.dph[1][NDPH-1],
	     optimize_control.sta[0],
	     optimize_control.sta[1]);
    }
    if (sc) {      
      SetScreenConfig(iter);
      double dz = fabs(potential->nbs-az0);
      double dzf = fabs(potential->nqf-af0);
      dz = Max(dz, dzf);
      double imx = -4.5/log(1-optimize_control.sta[1]);      
      if (imx < 2*NDPH) imx = 2*NDPH;
      if (imx > 5*NDPH) imx = 5*NDPH;
      if (_scpot.md >= 0) imx = 0;
      sta = optimize_control.sta[1];
      double sta01 = sta*0.01;
      muconv = fabs(potential->bps-au0)/Max(fabs(au0),0.5) < sta01;
      if (ahx < 1e-5 && _aaztol > 0 && iter > imx && tol < sta01 &&
	  ((dz < _aaztol*sta01) ||
	   (dz < _aaztol && muconv))) {
	break;
      }    
      az0 = potential->nbs;
      af0 = potential->nqf;
      au0 = potential->bps;
    } else {
      muconv = 1;
    }
    if (potential->miter > 0) break;
    sta = Min(optimize_control.sta[0], optimize_control.sta[1]);
    iter++;
    if (iter > miter) break;
  }
  for (i = 0; i < acfg->n_shells; i++) {
    if (!freeze[i]) continue;
    k = OrbitalExists(acfg->n[i], acfg->kappa[i], 0.0);
    orb = GetOrbital(k);
    free(orb->wfun);
    orb->wfun = NULL;
    orb->isol = 0;
    ierr = SolveDirac(orb);
    if (ierr < 0) {
      return -1;
    }
  }
  free(freeze);
  if (_print_maxiter) {
    printf("OptimizeLoop Max Iter: %4d\n", iter);
  }
  potential->miter = iter;
  potential->ihx = ihx0;
  return iter;
}

int OptimizeLoop(AVERAGE_CONFIG *acfg) {
  int iter = 0, imax;
  imax = optimize_control.maxiter;
  iter = OptimizeILoop(acfg, 0, imax, -1E31, 1E31, 1);
  if (potential->mps >= 0) potential->sps++;
  return iter;
} 
  
void CopyPotentialOMP(int init) {
  SetReferencePotential(hpotential, potential, 1);
  SetReferencePotential(rpotential, potential, 0);
#if USE_MPI == 2
  if (!MPIReady()) {
    InitializeMPI(0, 0);
    return;
  }
  POTENTIAL pot;
  pot.maxrp = 0;
  pot.nws = 0;
  pot.dws = NULL;
  CopyPotential(&pot, potential);
#pragma omp parallel shared(pot)
  {
    if (MyRankMPI() != 0) {  
      if (potential == NULL || potential->maxrp != pot.maxrp) {
	AllocDWS(pot.maxrp);
      }
      if (init) {
	potential = (POTENTIAL *) malloc(sizeof(POTENTIAL));
	potential->maxrp = 0;
      }
    }
    CopyPotential(potential, &pot);
  }
  CopyPotential(&pot, hpotential);
#pragma omp parallel shared(pot)
  {
    if (init && MyRankMPI() != 0) {
      hpotential = (POTENTIAL *) malloc(sizeof(POTENTIAL));
      hpotential->maxrp = 0;
    }
    CopyPotential(hpotential, &pot);
  }
  CopyPotential(&pot, rpotential);
#pragma omp parallel shared(pot)
  {
    if (init && MyRankMPI() != 0) {
      rpotential = (POTENTIAL *) malloc(sizeof(POTENTIAL));
      rpotential->maxrp = 0;
    }
    CopyPotential(rpotential, &pot);
  }
  if (pot.maxrp > 0) free(pot.dws);
#endif
}

static double EnergyHXS(int *n, double *x) {
  int md = potential->mode/10;
  if (md == 3) {
    potential->hxs = x[0];
    potential->chx = x[1];
  } else if (md == 4) {
    potential->hxs = x[0];
  } else if (md == 5) {
    potential->chx = x[0];
  }
  if (potential->chx < 1e-4 || potential->chx > 1e2) return 1e30;
  if (potential->hxs < 1e-4 || potential->hxs > 1e2) return 1e30;
  if (_refine_msglvl > 0) {
    printf("EnergyHXS: %2d %12.5E %12.5E ... ",
	   md, potential->hxs, potential->chx);
  }
  ReinitRadial(1);
  int iter = OptimizeLoop(&average_config);
  double a;
  if (average_config.ng > 0) {
    a = TotalEnergyGroups(average_config.ng, average_config.kg, NULL);
  } else {
    a = AverageEnergyAvgConfig(&average_config);
  }
  if (_refine_msglvl > 0) {
    printf("%3d %15.8E\n", iter, a);
  }
  return a;
}

void SetPotentialN(void) {
  double a = 0.0, b = 0.0;
  int i;
  AVERAGE_CONFIG *acfg = &(average_config);

  for (i = 0; i < acfg->n_shells; i++) {
    a += acfg->nq[i];
    if (i < acfg->n_cores) {
      b += acfg->nq[i];
    }
    if (optimize_control.iprint > 1) {
      MPrintf(-1, "avgcfg: %3d %3d %3d %11.4E %11.4E %11.4E %11.4E\n",
	      i, acfg->n[i], acfg->kappa[i], acfg->nq[i], a, b, acfg->e[i]);
    }
  }
  if (a > 1 && potential->mps < 0) {
    a += potential->zps;
  }
  if (a > potential->atom->atomic_number) {
    a = potential->atom->atomic_number/a;
    for (i = 0; i < acfg->n_shells; i++) {
      acfg->nq[i] *= a;
    }
    b *= a;
    a = potential->atom->atomic_number;
  }
  potential->N = a;
  potential->NC = b;
  potential->rhx = 0;
  potential->dhx = 0;
  if (fabs(potential->ihx) < EPS10 &&
      potential->N >= potential->atom->atomic_number-_ihxmin) {
    potential->ihx = -(potential->N-potential->atom->atomic_number+_ihxmin);
  }
  if (potential->ihx > 0) {
    a = 0.0;
    for (i = 0; i < acfg->n_shells; i++) {
      a += Min(acfg->nq[i], 1);
    }
    for (i = 0; i < acfg->n_shells; i++) {
      acfg->nq[i] -= potential->ihx*Min(acfg->nq[i],1)/a;
    }
  }
  potential->N1 = potential->N - fabs(potential->ihx);
  if (potential->N1 < 0) potential->N1 = 0;
}

int OptimizeRadial(int ng, int *kg, int ic, double *weight, int ife) {
  if (strlen(_sc_cfg) < 2) {
    return OptimizeRadialWSC(ng, kg, ic, weight, ife);
  }
  int lepton_type = LEPTON_TYPE;
  double lepton_mass = LEPTON_MASS*ELECTRON_MASS;
  double lepton_charge = LEPTON_CHARGE;
  int niter = 0;
  int i, ns, nd;
  int *n, *kappa;
  double *nq;
  double *dg;
  int done;
  double dm, dd, qs;
  ns = GetAverageConfigFromString(&n, &kappa, &nq, _sc_cfg);
  if (ns <= 0) {
    printf("invalid screen config: %s\n", _sc_cfg);
    return -1;
  }
  qs = 0.0;
  for (i = 0; i < ns; i++) {
    qs += nq[i];
  }
  dg = NULL;
  done = 0;

  int maxrp0 = potential->maxrp;
  int maxrp = (int)(_sc_fmrp*DMAXRP);
  maxrp = Max(maxrp, maxrp0);
  double z = GetAtomicNumber();
  double rn = GetAtomicR();
  double rmin, rmax, rmin1, rmax1;
  double a, b, c;
  b = RBOHR/_RBOHR;
  rmin = potential->rmin/z;
  if (rn > 0) {
    a = rn*GRIDRMINN0;
    if (rmin < a) rmin = a;
    a = rn*GRIDRMINN1;
    if (rmin > a) rmin = a;
  }
  MaxRGrid(potential, potential->asymp, potential->ratio, z,
	   rmin, maxrp, &a, &c, &rmax);
  rmin *= b;
  rmax *= b;
  SetLepton(_sc_lepton, _sc_mass, _sc_charge, NULL);
  b = _RBOHR/RBOHR;
  rmin1 = rmin*b;
  a = 1+z-qs;
  c = 0.025*z;
  a = Max(a, c);
  MaxRGrid(potential, potential->asymp, potential->ratio, a,
	   rmin1, maxrp, &a, &c, &rmax1);
  rmax1 /= b;
  rmax = Max(rmax, rmax1);
  SetLepton(lepton_type, lepton_mass, lepton_charge, NULL);
  b = _RBOHR/RBOHR;
  rmin *= z;
  while (1) {    
    SetRadialGrid(maxrp, GRIDRATIO, -rmax*b, rmin*b, potential->qr);
    if (0 > OptimizeRadialWSC(ng, kg, ic, weight, ife)) {
      return -1;
    }
    if (done) break;
    SaveSCPot(3, NULL, 0.0);
    ReinitRadial(0);
    SetLepton(_sc_lepton, _sc_mass, _sc_charge, NULL);
    b = _RBOHR/RBOHR;
    SetRadialGrid(maxrp, GRIDRATIO, -rmax*b, rmin*b, potential->qr);
    SetAverageConfig(ns, n, kappa, nq);
    if (0 > OptimizeRadialWSC(0, NULL, -1, NULL, 0)) {
      return -1;
    }
    a = (qs+potential->zps)/potential->N;
    SaveSCPot(3, NULL, a);
    if (niter == 0) {
      dg = malloc(sizeof(double)*_scpot.nr);
    } else {
      dm = 0;
      for (i = 0; i < _scpot.nr; i++) {
	if (_scpot.dg[i] > dm) dm = _scpot.dg[i];
      }
      dm *= 1e-3;
      dd = 0.0;
      nd = 0;
      for (i = 0; i < _scpot.nr; i++) {
	if (_scpot.dg[i] < dm) continue;
	dd += fabs(_scpot.dg[i]-dg[i]);
	nd++;
      }
      dd /= nd;
      if (dd < _sc_tol) {
	done = 1;
      }
    }
    memcpy(dg, _scpot.dg, sizeof(double)*_scpot.nr);
    ReinitRadial(0);
    SetLepton(lepton_type, lepton_mass, lepton_charge, NULL);
    niter++;
    if (niter > _sc_maxiter) {
      printf("sc_maxiter reached: %d\n", niter);
      break;
    }
  }
  free(dg);
  free(n);
  free(nq);
  free(kappa);
  return 0;
}

int OptimizeRadialWSC(int ng, int *kg, int ic, double *weight, int ife) {
  AVERAGE_CONFIG *acfg;
  double a, b, c, z, z0, rn;
  double *r, *x, emin, smin, hxs[NXS2], ehx[NXS2], mse;
  int iter, i, j, i0, i1, k, wce;
  
  if (potential->atom->atomic_number < EPS10) {
    printf("SetAtom has not been called\n");
    Abort(1);
  }
  wce = 0;
  if (!ife) {
    if (_config_energy >= 0) {
      ConfigEnergy(0, _config_energy, 0, NULL);
      wce = 1;
    } else {
      int **og = GetOptGrps(&k);
      if (k > 0) {
	ConfigEnergy(0, 0, 0, NULL);
	wce = 1;
      }
    }
  }
  mse = qed.se;
  qed.se = -1000000;
  /* get the average configuration for the groups */
  acfg = &(average_config);
  if (ng > 0) {
    if (acfg->n_allocs > 0) {      
      acfg->n_cfgs = 0;
      acfg->n_shells = 0;
      acfg->n_cores = 0;
      acfg->n_allocs = 0;
      free(acfg->n);
      free(acfg->kappa);
      free(acfg->nq);
      free(acfg->e);
      if (acfg->ng > 0) {
	free(acfg->kg);
	free(acfg->weight);
	acfg->ng = 0;
      }
      acfg->n = NULL;
      acfg->nq = NULL;
      acfg->e = NULL;
      acfg->kappa = NULL;
      acfg->kg = NULL;
      acfg->weight = NULL;
    }
    GetAverageConfig(ng, kg, ic, weight, _acfg_wmode,
		     optimize_control.n_screen,
		     optimize_control.screened_n,
		     optimize_control.screened_charge,
		     optimize_control.screened_kl, acfg); 
  } else if (potential->mps < 0 && _scpot.nr == 0) {
    if (acfg->n_shells <= 0) {
      printf("No average configuation exist. \n");
      printf("Specify with AvgConfig, ");
      printf("or give config groups to OptimizeRadial.\n");
      qed.se = mse;
      return -1;
    }
  }
  
  SetPotentialN();
  /* setup the radial grid if not yet */
  if (potential->flag == 0) {
    if (_gridemax > 0) {
      z0 = GetAtomicNumber();
      z = z0 - potential->N1;
      if (z < 1) z = 1.0;
      a = 10*sqrt(2*_gridemax*(1+0.5*FINE_STRUCTURE_CONST2*_gridemax)/z);
      potential->asymp = ceil(a);
      if (potential->asymp < GRIDASYMP) potential->asymp = GRIDASYMP;
    }
    if (_gridrmax > 0 || _gridrmax < -0.1) {
      if (potential->asymp > 0 && potential->ratio > 0) {
	z0 = GetAtomicNumber();
	rn = GetAtomicR();
	z = z0 - potential->N1;
	if (z < 1) z = 1.0;
	b = _gridrmax;
	if (b < 0) {
	  k = (int)ceil(-b);
	  b = 20*b*b/z;
	  for (i = 0; i < 100; i++) {
	    RadialDiracCoulomb(1, &a, &c, &b, z, k, -1);
	    a = sqrt(a*a+c*c);
	    if (a < EPS5) break;
	    b *= 1.25;
	  }
	}
	smin = potential->rmin/z0;
	if (rn > 0) {
	  a = rn*GRIDRMINN0;
	  if (smin < a) smin = a;
	  a = rn*GRIDRMINN1;
	  if (smin > a) smin = a;
	}
	a = potential->asymp*sqrt(2.0*z)/PI;
	c = 1./log(potential->ratio);
	k = (int)(a*(pow(b,potential->qr)-pow(smin,potential->qr)));
	k += (int)ceil(c*log(b/smin));
	k += 10;
	if (IsOdd(k)) k++;
	if (k > MMAXRP) {
	  printf("maxrp exceeded %d: %d %g %g %g\n", MMAXRP,
		 k, b, potential->ratio, potential->asymp);
	}
	AllocWorkSpace(k);
      }
    }
    SetOrbitalRGrid(potential);    
  }

  int nmax = potential->nmax-1;
  if (potential->nb > 0 && nmax < potential->nb) nmax = potential->nb;
  for (i = 0; i < acfg->n_shells; i++) {
    if (potential->mps < 0 && acfg->n[i] > nmax) {
      printf("too large n in avgcfg: %d %d %d %d %d %d %g\n",
	     ife, nmax, potential->nb,
	     i, acfg->n[i], acfg->kappa[i], acfg->nq[i]);
      if (ife) {
	return -1;
      }
      j = GetLFromKappa(acfg->kappa[i]);
      acfg->n[i] = potential->nmax;
      if (j/2 >= acfg->n[i]) {
	acfg->kappa[i] = -1;
      }
    }
  }
  
  SetPotentialZ(potential);
  
  if (_scpot.nr > 0) {
    a = _RBOHR/RBOHR;
    for (i = 0; i < _scpot.nr; i++) {
      _scpot.rg[i] *= a;
      if (_scpot.md >= 2) {
	_scpot.dg[i] /= a;
	_scpot.eg[i] /= a;
      }
    }
    r = potential->rad;
    x = potential->rho;
    for (i = 0; i < _scpot.nr; i++) {
      _scpot.xg[i] = potential->ar*pow(_scpot.rg[i],potential->qr) +
	potential->br*log(_scpot.rg[i]);
    }
    a = _scpot.rg[_scpot.nr-1];
    b = _scpot.rg[0];
    i0 = -1;
    for (i = 0; i < potential->maxrp; i++) {
      if (r[i] < b) i0 = i;
      if (r[i] > a) {
	i--;
	break;
      }
    }
    i0++;
    i1 = i;
    if (i1 >= potential->maxrp) i1 = potential->maxrp-1;
    _scpot.jmax = i1;
    i = i1-i0+1;
    switch (_scpot.md) {
    case 0:
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.dg, i, x+i0, _dphasep+i0);
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.eg, i, x+i0, _phase+i0);
      for (k = 0; k < i0; k++) {
	_dphasep[k] = _dphasep[i0]*r[k]/r[i0];
	_phase[k] = _phase[i0]*r[k]/r[i0];
      }
      for (k = i1+1; k < potential->maxrp; k++) {
	_dphasep[k] = _dphasep[i1];
	_phase[k] = _phase[i1];
      }
      break;
    case 1:
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.dg, i, x+i0, potential->ZPS+i0);
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.eg, i, x+i0, _zk+i0);
      for (k = 0; k < i0; k++) {
	potential->ZPS[k] = potential->ZPS[i0]*r[k]/r[i0];
	_zk[k] = _zk[i0]*r[k]/r[i0];
      }
      for (k = i1+1; k < potential->maxrp; k++) {
	potential->ZPS[k] = potential->ZPS[i1];
	_zk[k] = _zk[i1];
      }
      for (k = 0; k < potential->maxrp; k++) {
	potential->VXF[k] = 0.0;
	potential->EPS[k] = 0.0;
	_phase[k] = 0.0;
      }
      potential->zps = potential->ZPS[potential->maxrp-1];
      potential->nps = 1.0;
      break;
    case 2:
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.dg, i, x+i0, potential->NPS+i0);
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.eg, i, x+i0, _phase+i0);
      if (i0 > 0) {
	k = Min(i,5);
	if (potential->NPS[i0] > 0 && potential->NPS[i0+k] > 0) {
	  a = log(potential->NPS[i0+k]/potential->NPS[i0])/log(r[i0+k]/r[i0]);
	} else {
	  a = 2.0;
	}
	if (_phase[i0] > 0 && _phase[i0+k] > 0) {
	  b = log(_phase[i0+k]/_phase[i0])/log(r[i0+k]/r[i0]);
	} else {
	  b = 2.0;
	}
	for (k = 0; k < i0; k++) {
	  potential->NPS[k] = potential->NPS[i0]*pow(r[k]/r[i0],a);
	  _phase[k] = _phase[i0]*pow(r[k]/r[i0], b);
	}
      }
      for (k = i1+1; k < potential->maxrp; k++) {
	potential->NPS[k] = 0.0;
	_phase[k] = 0.0;
      }
      for (k = 0; k < potential->maxrp; k++) {
	_dwork1[k] = potential->NPS[k]*potential->dr_drho[k];
      }
      a = 0.0;
      b = 0.0;
      if (i0 > 0) {
	a = Simpson(_dwork1, 0, i0);
      }
      if (i1 > i0) {
	b = Simpson(_dwork1, i0, i1);
      }
      if (b > 0) {
	a = b/(a+b);
	for (k = 0; k < potential->maxrp; k++) {
	  potential->NPS[k] *= a;
	  _phase[k] *= a;
	}
      }
      potential->zps = b;
      potential->nps = 1.0;
      break;
    default:
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.dg, i, x+i0, potential->VPS+i0);
      UVIP3P(3, _scpot.nr, _scpot.xg, _scpot.eg, i, x+i0, potential->EPS+i0);
      if (i0 > 0) {
	k = Min(i,5);
	if (potential->VPS[i0] > 0 && potential->VPS[i0+k] > 0) {
	  a = log(potential->VPS[i0+k]/potential->VPS[i0])/log(r[i0+k]/r[i0]);
	} else {
	  a = 2.0;
	}
	if (potential->EPS[i0] > 0 && potential->EPS[i0+k] > 0) {
	  b = log(potential->EPS[i0+k]/potential->EPS[i0])/log(r[i0+k]/r[i0]);
	} else {
	  b = 2.0;
	}
	for (k = 0; k < i0; k++) {
	  potential->VPS[k] = potential->VPS[i0]*pow(r[k]/r[i0],a);
	  potential->EPS[k] = potential->EPS[i0]*pow(r[k]/r[i0],b);
	}
      }
      for (k = i1+1; k < potential->maxrp; k++) {
	potential->VPS[k] = 0.0;
	potential->EPS[k] = 0.0;
      }
      for (k = 0; k < potential->maxrp; k++) {
	potential->VXF[k] = potential->VPS[k];
	_dwork1[k] = potential->VPS[k]*potential->dr_drho[k];
      }
      a = 0.0;
      b = 0.0;
      if (i0 > 0) {
	a = Simpson(_dwork1, 0, i0);
      }
      if (i1 > i0) {
	b = Simpson(_dwork1, i0, i1);
      }
      if (b > 0) {
	a = b/(a+b);
	for (k = 0; k < potential->maxrp; k++) {
	  potential->VPS[k] *= a;
	  potential->VXF[k] *= a;
	  potential->EPS[k] *= a;
	}
      }
      potential->zps = b;
      //printf("zps: %d %d %g %g %g %g\n", i0, i1, potential->N, potential->zps, a, b);
      potential->nps = 1.0;
      break;
    }
    SetPotentialN();
  }
  SetReferencePotential(hpotential, potential, 1);
  SetReferencePotential(rpotential, potential, 0);
  z = GetResidualZ();
  potential->a = 0.0;
  potential->lambda = 0.5*z;
  if (potential->N > 1) {
    potential->r_core = potential->maxrp-5;
  } else {
    potential->r_core = 50;
  }

  if (optimize_control.iset == 0) {
    double z0 = potential->atom->atomic_number;
    optimize_control.stabilizer = 0.25 + 0.5*(z/z0);
  }

  int md = potential->mode/10;
  if (md > 2) {
    int n = 1;
    if (md == 3) n = 2;    
    double xtol = 1e-2;
    int mode = 0;
    int *lw = malloc(sizeof(int)*n*2);
    double *x = malloc(sizeof(double)*n);
    double *scale = malloc(sizeof(double)*n);
    double *dw = malloc(sizeof(double)*(n*2+n*(n+4)+1));
    double f;
    for (i = 0; i < n; i++) {
      x[i] = 1.0;
      scale[i] = 0.1;
    }
    int nfe = 0;
    int ierr = 0;
    int maxfun = _refine_maxfun;
    potential->hxs = 1.0;
    potential->chx = 1.0;
    SUBPLX(EnergyHXS, n, xtol, maxfun, mode, scale, x,
	   &f, &nfe, dw, lw, &ierr);
    free(x);
    free(scale);
    free(lw);
    free(dw);
  } else if (md == 2) {
    hxs[NXS] = 1.0;
    for (i = NXS-1; i >= 0; i--) {
      hxs[i] = hxs[i+1] - 0.05;
    }
    for (i = NXS+1; i < NXS2; i++) {
      hxs[i] = hxs[i-1] + 0.05;
    }
    i = NXS;    
    potential->hxs = hxs[i];
    iter = OptimizeLoop(acfg);
    if (iter > optimize_control.maxiter) {
      printf("Maximum iteration reached in OptimizeRadial %d %d\n", i, iter);
      return -1;
    }
    if (ng > 0) {
      ehx[i] = TotalEnergyGroups(ng, kg, NULL);
    } else {
      ehx[i] = AverageEnergyAvgConfig(acfg);
    }
    if (optimize_control.iprint) {
      printf("hxs iter: %d %d %g %g %18.10E\n", ng, i, hxs[i], potential->ahx, ehx[i]);
    }
    int im = NXS;
    double em = ehx[im];
    for (i = NXS-1; i >= 0; i--) {
      ReinitRadial(1);
      potential->hxs = hxs[i];
      iter = OptimizeLoop(acfg);
      if (iter > optimize_control.maxiter) {
	printf("Maximum iteration reached in OptimizeRadial %d %d\n", i, iter);
	return -1;
      }
      if (ng > 0) {
	ehx[i] = TotalEnergyGroups(ng, kg, NULL);
      } else {
	ehx[i] = AverageEnergyAvgConfig(acfg);
      }
      if (optimize_control.iprint) {
	printf("hxs iter: %d %d %g %g %18.10E\n", ng, i, hxs[i], potential->ahx, ehx[i]);
      }
      if (ehx[i] < em) {
	em = ehx[i];
	im = i;
      } else if (im-i > 1) {
	break;
      }
    }
    i0 = Max(i, 0);    
    for (i = NXS+1; i < NXS2; i++) {
      ReinitRadial(1);
      potential->hxs = hxs[i];
      iter = OptimizeLoop(acfg);
      if (iter > optimize_control.maxiter) {
	printf("Maximum iteration reached in OptimizeRadial %d %d\n", i, iter);
	return -1;
      }
      if (ng > 0) {
	ehx[i] = TotalEnergyGroups(ng, kg, NULL);
      } else {
	ehx[i] = AverageEnergyAvgConfig(acfg);
      }
      if (optimize_control.iprint) {
	printf("hxs iter: %d %d %g %g %18.10E\n", ng, i, hxs[i], potential->ahx, ehx[i]);
      }
      if (ehx[i] < em) {
	em = ehx[i];
	im = i;
      } else if (i-im > 0) {
	break;
      }
    }
    i1 = i;
    if (i1 >= NXS2) i1 = NXS2-1;
    emin = 1e10;
    smin = 0;
    j = 0;
    for (i = i0; i <= i1; i++) {
      if (ehx[i] < emin) {
	emin = ehx[i];
	j = i;
      }
    }
    k = i1 - i0 + 1;
    b = 0.001;
    i = j;
    if (i < i1) i++;
    if (j > i0) j--;
    a = hxs[j];
    emin = 1e10;
    smin = 0.0;
    while (a <= hxs[i]) {
      UVIP3P(3, k, hxs+i0, ehx+i0, 1, &a, &c);
      if (c < emin) {
	emin = c;
	smin = a;
      }
      a += b;
    }
    
    ReinitRadial(1);
    potential->hxs = smin;
    
    if (optimize_control.iprint) {
      printf("hxs: %g %18.10E\n", smin, emin);
    }
    iter = OptimizeLoop(acfg);
  } else {
    iter = OptimizeLoop(acfg);
  }
  qed.se = mse;
  CopyPotentialOMP(0);
  if (wce) {
    ConfigEnergy(1, 0, 0, NULL);
  }
  return iter;
}      

double NBoundAA(int ns, int *n, int *ka, double *nq, double *et, double *nqc,
		double u, double tps, double e0, double *nqf) {
  int ik, j2;
  double x, y, nb, ek;
  nb = 0.0;
  for (ik = 0; ik < ns; ik++) {
    if ((et[ik] >= 1e30) || _znbaa > 0) {
      nq[ik] = 0.0;
      continue;
    }
    x = et[ik]-u;
    if (x > 0) {
      y = exp(-x);
      y /= 1+y;
    } else {
      y = 1/(1+exp(x));
    }
    j2 = GetJFromKappa(ka[ik]);
    nq[ik] = (j2+1.0-nqc[ik])*y;
    ek = et[ik]*tps;    
    y = BoundFactor(ek, potential->eth, potential->ewd, potential->mps);
    nq[ik] *= y;    
    if (nq[ik] < 1e-50) nq[ik] = 0.0;
    nb += nq[ik];
  }
  if (nqf) {
    if (e0 < 1E31) {
      *nqf = FreeElectronDensity(potential, e0, u, 0.0, 2, 0);
    }
    nb += *nqf;
  }
  return nb;
}

double OptimizeAA(double zb, int m, double d, double t, int it) {
  ReinitRadial(0);
  PlasmaScreen(20+m, _sc_vxf, zb, zb*d, t, zb);
  OptimizeRadial(0, NULL, -1, NULL, 0);

  return potential->aps-potential->bps;
}

double EffectiveTe(double ne, double ke) {
  double u = FreeEta1(ne, ke);
  return FreeTe(u, ne);
}

void AverageAtom(char *pref, int m, double d0, double t, double ztol) {
  char pfn[1024], dfn[1024], sfn[1024];  
  int it, nm, ik, k, k0, k1, idx;
  double z0, zb0, zb1, a, b, c, d, e, x, y, r, nb, vc, zbr;
  double v, epsr, epsa, u, u0, u1, ke, km, tm, etf, ex, dn;
  ORBITAL *orb;

  _aaztol = ztol;
  if (ztol < 0.0) _aaztol = 0.001;
  dn = d0/(potential->atom->mass*AMU*9.10938356e-4);
  z0 = potential->atom->atomic_number;
  if (m >= 3) {
    PlasmaScreen(m, _sc_vxf, z0, dn, t, -1.0);
    OptimizeRadial(0, NULL, -1, NULL, 0);
  } else {
    epsr = 1e-5;
    epsa = 1e-3;
    b = z0/2.0;
    it = 0;
    a = OptimizeAA(b, m, dn, t, 0);
    if (a > 0) {
      while (a > 0) {
	zb1 = b;
	u1 = a;
	b = 0.5*b;      
	a = OptimizeAA(b, m, dn, t, 0);
	it++;
	if (it > optimize_control.maxiter) {
	  printf("maxiter reached in AverageAtom loop1: %d %g %g\n", it, a, b);
	  Abort(1);
	}
      }
      zb0 = b;
      u0 = a;
    } else if (a < 0) {
      while (a < 0) {
	zb0 = b;
	u0 = a;
	b += 0.5*(z0-b);
	a = OptimizeAA(b, m, dn, t, 0);
	it++;
	if (it > optimize_control.maxiter) {
	  printf("maxiter reached in AverageAtom loop2: %d %g %g\n", it, a, b);
	  Abort(1);
	}	
      }
      zb1 = b;
      u1 = a;
    }    
    it = 0;
    while ((fabs(zb1-zb0) > epsa &&
	    fabs(zb1-zb0) > epsr*zb1) &&
	   fabs(u1-u0) > 1e-3) {
      u = u1/(u1-u0);
      if (u < 0.25) u = 0.25;
      if (u > 0.75) u = 0.75;
      b = u*zb0 + (1-u)*zb1;
      a = OptimizeAA(b, m, dn, t, it);
      printf("avg atom: %d %g %g %g %g %g %g %g\n",
	     it, zb0, zb1, u0, u1, b, a, potential->aps);
      if (fabs(a) < 1e-4) break;
      if (a < 0) {
	zb0 = b;
	u0 = a;
      } else {
	zb1 = b;
	u1 = a;
      }
      it++;
      if (it > optimize_control.maxiter) {
	printf("maxiter reached in AverageAtom loop3: %d %g %g %g %g\n",
	       it, zb0, zb1, a, b);
      }      
    }    
  }
  _aaztol = 0.0;
  sprintf(pfn, "%s.pot", pref);
  sprintf(dfn, "%s.den", pref);
  sprintf(sfn, "%s.dos", pref);
  GetPotential(pfn, 0);
  FILE *f;
  f = fopen(dfn, "w");
  if (f == NULL) {
    printf("cannot open file %s\n", dfn);
    Abort(1);
  }
  for (k = 0; k < potential->maxrp; k++) {
    _dwork1[k] = 0.0;
    _dwork2[k] = 0.0;
    _dwork6[k] = 0.0;
    _zk[k] = potential->EPS[k];
  }
  if (potential->nbt < 1e-10) {
    u = potential->aps;
  } else {
    u = potential->bps;
  }
  if (SCEWM() == 0) {
    etf = -1E31;
  } else {
    etf = potential->eth;
  }
  a = FreeElectronDensity(potential, etf, potential->bps, 0.0, 4, 0);
  for (k = 0; k < potential->maxrp; k++) {
    _dwork5[k] = potential->EPS[k];
    _dwork17[k] = _dwork5[k];
  }
  a = FreeElectronDensity(potential, etf, potential->bps, 0.0, 10, 0);
  for (k = 0; k < potential->maxrp; k++) {
    _dwork7[k] = potential->EPS[k];
  }
  double nft = FreeElectronDensity(potential, potential->efm, u, 0.0, 2, 0);
  if (nft < 1e-99) nft = 0.0;
  for (k = 0; k < potential->maxrp; k++) {
    _dwork[k] = potential->EPS[k];
    potential->EPS[k] = _zk[k];    
    _dwork3[k] = 0.0;
    _dwork4[k] = 0.0;
    _xk[k] = _dwork5[k];
    _yk[k] = 0.0;
  }
  AVERAGE_CONFIG *ac = &(average_config);
  nm = 0;
  char orbf[128];
  k1 = potential->ips;
  nb = 0.0;
  vc = potential->VT[0][k1];
  for (ik = 0; ik < ac->n_shells; ik++) {
    idx = OrbitalIndex(ac->n[ik], ac->kappa[ik], 0);
    orb = GetOrbitalSolved(idx);
    b = (orb->energy/potential->tps)-potential->bps;
    if (b > 0) {
      b = exp(-b);
      b /= 1+b;
    } else {
      b = 1/(1+exp(b));
    }
    c = 2*abs(ac->kappa[ik])*b;
    if (orb->energy < vc) {
      nm++;
      nb += c;
    }
    if (orb->ilast < k1) k1 = orb->ilast;
    for (k = 0; k <= orb->ilast; k++) {
      x = Large(orb)[k];
      y = Small(orb)[k];
      r = x*x;
      a = (r + y*y);
      d = c*a;
      a = ac->nq[ik]*a;
      r = ac->nq[ik]*r;
      _dwork1[k] += a;
      if (orb->energy < 0) {
	_dwork3[k] += d*log(-orb->energy);
	_dwork6[k] += a;
      }
      _dwork4[k] += a;
      _dwork5[k] += a*(orb->energy-potential->VT[0][k]);
      _xk[k] += r*(orb->energy-potential->VT[0][k]);
      _yk[k] += r;
      if (b > 0) {
	_dwork7[k] += -a*b*log(b);      
	if (b < 1.0) {
	  _dwork7[k] += -a*(1-b)*log(1-b);
	}
      }
    }
  }
  k0 = k1-3;
  k0 = Max(1, k0);
  b = 0.0;
  for (k = k0; k <= k1; k++) {
    x = potential->rad[k];
    x *= x;
    a = (_dwork4[k] + potential->EPS[k])/x;
    b += a;
  }
  a = 1.+k1-k0;
  b /= a;
  zb1 = b*pow(potential->rps,3)/3.0;
  x = b/FOUR_PI;
  u1 = FermiDegeneracy(x, potential->tps, &u0);
  zb0 = FreeElectronIntegral(potential, 0, potential->ips, potential->maxrp-1,
			     _dphasep, NULL, 0.0, potential->tps,
			     potential->eth, 0.0, u1, 0.0, 2, 0,
			     0.0, 0.0, 0.0, 0.0, NULL);
  zb0 = 0.0;
  for (k = k0; k <= k1; k++) {
    x = potential->rad[k];
    x *= x;
    zb0 += _dphasep[k]/x;
  }
  zb0 /= a;
  zbr = 0.0;
  if (zb0 > 0) {
    zbr = b/zb0;
    for (k = 0; k <= potential->ips; k++) {
      _dwork2[k] = _dphasep[k]*zbr;
    }
  }
  a = 0.0;
  ik = potential->ips;
  for (k = 0; k <= potential->ips; k++) {
    _dwork14[k] = _dwork4[k] + potential->EPS[k];
    if (k < potential->ips && _dwork2[k] > _dwork14[k]) {
      b = potential->rad[k]/potential->rad[potential->ips];
      b = log(_dwork14[k]/_dwork2[k])/log(b);
      if (b > a) {
	a = b;
	ik = k;
      }
    }
  }
  for (k = 0; k <= potential->ips; k++) {
    if (k > ik) {
      _dwork2[k] = _dwork14[k];
    } else if (a > 0) {
      b = potential->rad[k]/potential->rad[potential->ips];
      _dwork2[k] *= pow(b, a);
    }
    _dwork1[k] = _dwork14[k] - _dwork2[k];
    _zk[k] = _dwork1[k]*potential->dr_drho[k];
  }
  zb0 = Simpson(_zk, 0, potential->ips);
  tm = potential->tps*_minke;
  for (k = 0; k <= potential->ips; k++) {
    if (fabs(_dwork4[k]) < 1e-90) _dwork4[k] = 0.0;
    a = (potential->EPS[k]+_dwork4[k]);
    b = (potential->EPS[k]+_yk[k]);
    _dwork14[k] = a;
    _dwork15[k] = a;
    if (a > 0) {
      _dwork3[k] = exp(2*_dwork3[k]/a)*potential->rad[k]*potential->rad[k];
      if (fabs(_dwork3[k]) < 1e-90) _dwork3[k] = 0.0;
      _dwork7[k] /= a;
      _dwork7[k] *= potential->tps;
      _dwork5[k] /= a;
      if (fabs(_dwork7[k]) < 1e-90) _dwork7[k] = 0.0; 
      if (fabs(_dwork5[k]) < 1e-90) _dwork5[k] = 0.0;         
      b = potential->rad[k];
      b = a/(FOUR_PI*b*b);
      ke = _dwork5[k];
      u = FreeEta0(b, tm);
      km = FreeKe(u, tm);
      if (ke < km) ke = km;
      u = FreeEta1(b, ke);
      _dwork12[k] = FreeTe(u, b);
      _dwork13[k] = u;
    } else {
      _dwork3[k] = 0.0;
      _dwork5[k] = 0.0;
      _dwork7[k] = 0.0;
      _dwork12[k] = 0.0;
      _dwork13[k] = 0.0;
    }
  }
  if (potential->vxm > 0) {
    for (k = 0; k <= potential->ips; k++) {
      r = potential->rad[k];
      b = _dwork14[k]*potential->dr_drho[k];
      c = _dwork14[k]/(FOUR_PI*r*r);
      _phase[k] = -XCPotential(potential->tps, c, 20+potential->vxm)*b;
    }
    ex = Simpson(_phase, 0, potential->ips)*HARTREE_EV;
  }
  k = DensityToSZ(potential, _dwork14, _dwork10, _dwork15, &b, 0);
  for (k = 0; k <= potential->ips; k++) {
    r = potential->rad[k];
    b = _dwork14[k]*potential->dr_drho[k];
    a = potential->Z[k];
    _xk[k] *= potential->dr_drho[k];
    _dwork17[k] *= potential->dr_drho[k];
    _dwork8[k] = b*_dwork5[k];
    _dwork9[k] = b*_dwork7[k];
    _dwork11[k] = -b*(a/r);
    _dphase[k] = b*_dwork10[k]/r;
    _dwork16[k] = -b*_dwork15[k]/r;
  }
  a = HARTREE_EV;
  x = Simpson(_dwork8, 0, potential->ips)*a;
  y = Simpson(_dwork9, 0, potential->ips)*a;
  u = 0.5*Simpson(_dphase, 0, potential->ips)*a;
  r = Simpson(_dwork11, 0, potential->ips)*a;
  e = Simpson(_xk, 0, potential->ips)*a;
  c = Simpson(_dwork17, 0, potential->ips)*a;
  b = Simpson(_dwork16, 0, potential->ips)*a;
  if (potential->vxm == 0) {
    ex = b*0.75;
  }
  fprintf(f, "#   d0: %15.8E\n", d0);
  fprintf(f, "#   dn: %15.8E\n", dn);
  fprintf(f, "#    T: %15.8E\n", t);
  fprintf(f, "#    Z: %15.8E\n", z0);
  a = z0-potential->nbs-potential->NC;
  a = Max(0.0, a);
  fprintf(f, "#   za: %15.8E\n", a);
  a = z0-nb;
  a = Max(0.0, a);
  fprintf(f, "#   zb: %15.8E\n", a);
  fprintf(f, "#   ze: %15.8E\n", zb1);
  a = z0-zb0;
  a = Max(0.0, a);
  fprintf(f, "#   zf: %15.8E\n", a);
  fprintf(f, "#   zr: %15.8E\n", zbr);
  fprintf(f, "#   nm: %15d\n", nm);
  fprintf(f, "#   uf: %15.8E\n", potential->aps);
  fprintf(f, "#   ub: %15.8E\n", potential->bps);
  fprintf(f, "#  efm: %15.8E\n", potential->efm);
  fprintf(f, "#  eth: %15.8E\n", potential->eth);
  fprintf(f, "#  ewd: %15.8E\n", potential->ewd);
  fprintf(f, "#  ewf: %15.8E\n", SCEWF());
  fprintf(f, "#  ewm: %15d\n", SCEWM());
  fprintf(f, "#  nqf: %15.8E\n", potential->nqf);
  fprintf(f, "#  zps: %15.8E\n", potential->zps);
  fprintf(f, "#  nps: %15.8E\n", potential->nps);
  fprintf(f, "#  tps: %15.8E\n", potential->tps);
  fprintf(f, "#  rps: %15.8E\n", potential->rps);
  fprintf(f, "#  dps: %15.8E\n", potential->dps);
  fprintf(f, "#  ntf: %15.8E\n", nft);
  fprintf(f, "#   ek: %15.8E\n", x);
  fprintf(f, "#   ec: %15.8E\n", c);
  fprintf(f, "#   ts: %15.8E\n", y);
  fprintf(f, "#   ve: %15.8E\n", u);
  fprintf(f, "#   vx: %15.8E\n", ex);
  fprintf(f, "#   ux: %15.8E\n", b);
  fprintf(f, "#   vn: %15.8E\n", r);
  fprintf(f, "#   vf: %15.8E\n", u+r+ex+x-y);
  fprintf(f, "#   ep: %15.8E\n", e);
  fprintf(f, "#   vp: %15.8E\n", u+r+2*e+3*(b-ex));
  fprintf(f, "#  rsf: %15.8E\n", SCRSF());
  fprintf(f, "#  rbf: %15.8E\n", SCRBF());
  fprintf(f, "#  bqp: %15.8E\n", SCBQP());
  fprintf(f, "#  vxf: %15d\n", _sc_vxf);
  fprintf(f, "#  vxm: %15d\n", potential->vxm);
  fprintf(f, "\n");
  fprintf(f, "## c00: index\n");
  fprintf(f, "## c01: radius\n");
  fprintf(f, "## c02: core density\n");
  fprintf(f, "## c03: valence density\n");
  fprintf(f, "## c04: TF density\n");
  fprintf(f, "## c05: exchange density\n");
  fprintf(f, "## c06: bound density\n");
  fprintf(f, "## c07: free density\n");
  fprintf(f, "## c08: over efm density\n");
  fprintf(f, "## c09: Non TF density\n");
  fprintf(f, "## c10: avg binding energy density\n");
  fprintf(f, "## c11: total kinetic energy\n");
  fprintf(f, "## c12: entropy\n");
  fprintf(f, "## c13: effective potential\n");
  fprintf(f, "## c14: nuclear potential\n");
  fprintf(f, "## c15: effective temperature\n");
  fprintf(f, "## c16: chemical potential\n");
  fprintf(f, "## c17: electron screening\n");
  fprintf(f, "## c18: exchange screening\n");
  fprintf(f, "## c19: negative energy density\n");
  fprintf(f, "## c20: TF kinetic energy\n");
  fprintf(f, "## c21: TF positive energy density\n");
  fprintf(f, "\n");

  for (k = 0; k <= potential->ips; k++) {
    a = potential->rad[k];
    fprintf(f, "%6d %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E\n",
	    k, a, potential->NPS[k], potential->VPS[k],
	    potential->EPS[k], potential->VXF[k],
	    _dwork1[k], _dwork2[k], _dwork[k], _dwork4[k], _dwork3[k],
	    _dwork5[k], _dwork7[k], potential->VT[0][k],
	    -potential->Z[k]/a, _dwork12[k], _dwork13[k],
	    _dwork10[k], _dwork15[k], _dwork6[k],
	    _dwork17[k]/potential->dr_drho[k], _dphasep[k]);
  }
  fclose(f);

  f = fopen(sfn, "w");
  if (f == NULL) {
    printf("cannot open file %s\n", sfn);
    Abort(1);
  }
  d = 15*potential->ewd;
  a = potential->eth - d;
  b = d*10;
  nm = (int)(1+(b-a)/(potential->ewd*0.25));
  d = (b-a)/(nm-1.0);
  c = 4/PI;
  e = a+0.5*d;
  for (k = 0; k < nm; k++) {
    for (ik = 0; ik < potential->maxrp; ik++) {
      v = potential->VT[0][ik];
      r = e-v;
      if (r > 0) {
	a = FINE_STRUCTURE_CONST*r;
	r = 2*r + a*a;
	b = 1-BoundFactor(e, potential->eth, potential->ewd, potential->mps);
	b *= c*sqrt(r)*d*(1 + FINE_STRUCTURE_CONST*a);
	r = potential->rad[ik];      
	_dwork[ik] = b*r*r*potential->dr_drho[ik];
      } else {
	_dwork[ik] = 0.0;
      }      
    }
    r = Simpson(_dwork, 0, potential->maxrp-1);
    fprintf(f, "%6d %15.8E %15.8E %15.8E\n", k, e, d, r);
    e += d;
  }  
  fclose(f);
}

static double EnergyFunc(int *n, double *x) {
  double a;
  int i, k, np;
  ORBITAL *orb;
  
  if (_refine_mode == 1) {
    for (i = 0; i < potential->maxrp; i++) {
      potential->U[i] = _dphasep[i];
      for (k = 0; k < *n; k++) {      
	potential->U[i] += x[k]*_refine_wfb[k][i];
      }
    }      
  } else if (_refine_mode == 2) {
    np = 2 + (*n);
    for (k = 0; k < *n; k++) {
      _refine_wfb[1][k+1] = x[k];
    }    
    spline(_refine_wfb[0], _refine_wfb[1], np, 0.0, 0.0, _refine_wfb[2]);
    for (i = 0; i < potential->maxrp; i++) {
      splint(_refine_wfb[0], _refine_wfb[1], _refine_wfb[2], np,
	     _refine_wfb[3][i], &a);
      potential->U[i] = _dphasep[i] + a/potential->rad[i];
    }
    /*
    UVIP3P(3, np, _refine_wfb[0], _refine_wfb[1],
	   potential->maxrp, _refine_wfb[3], _refine_wfb[4]);
    for (i = 0; i < potential->maxrp; i++) {
      potential->U[i] = _dphasep[i] + _refine_wfb[4][i]/potential->rad[i];
    }
    */
  }
  //SetPotentialVc(potential);
  SetPotentialU(potential, 0, NULL);
  SetPotentialVT(potential);
  ReinitRadial(1);
  ClearOrbitalTable(0);
  if (average_config.ng > 0) {
    if (_refine_pj < 0) {
      a = TotalEnergyGroups(average_config.ng, average_config.kg, NULL);
    } else {
      a = 0.0;
      if (_refine_em == 0) {
	ConstructHamiltonDiagonal(_refine_pj, average_config.ng,
				  average_config.kg, -1);
	HAMILTON *h = GetHamilton(_refine_pj);
	for (i = 0; i < h->dim; i++) {
	  a += h->hamilton[i];
	}
	AllocHamMem(h, -1, -1);
	AllocHamMem(h, 0, 0);
      } else {
	int ng = average_config.ng;
	int *kg = malloc(sizeof(int)*ng);
	memcpy(kg, average_config.kg, sizeof(int)*ng);
	SolveStructure(NULL, NULL, ng, kg, 0, NULL, 0);
	HAMILTON *h = GetHamilton(_refine_pj);
	a = h->mixing[0];
	AllocHamMem(h, -1, -1);
	AllocHamMem(h, 0, 0);
      }
    }
  } else {
    a = AverageEnergyAvgConfig(&average_config);
  }
  if (_refine_msglvl > 1) {
    printf("refine step: ");
    for (i = 0; i < *n; i++) {
      printf("%g ", x[i]);
    }
    printf("--- %18.10E\n", a);
  }
  return a;
}

int RefineRadial(int m, int n, int maxfun, int msglvl) {
  int i, k, ierr, mode, nfe, se, *lw, n2, nw;
  double xtol;
  double f0, f, *x, *scale, *dw;
  AVERAGE_CONFIG *a;
  ORBITAL *orb;

  if (potential->N < 1+EPS5) return 0;
  if (m == 0) m = 2;
  if (n == 0) n = _refine_np;
  if (maxfun <= 0) maxfun = _refine_maxfun;
  if (msglvl < 0) msglvl = _refine_msglvl;
  se = qed.se;
  qed.se = -1000000;
  double t0 = WallTime();
  if (maxfun <= 0) maxfun = 10000;
  if (n <= 0) n = 6;
  for (i = 0; i < potential->maxrp; i++) {
    _dphasep[i] = potential->U[i];
  }
  a = &average_config;
  if (m%2 == 1) {
    _refine_wfb = malloc(sizeof(double *)*n);
    nw = n;
    for (i = 0; i < n; i++) {
      _refine_wfb[i] = malloc(sizeof(double)*potential->maxrp);
      k = OrbitalIndex(i+1, -1, 0);
      orb = GetOrbitalSolved(k);
      for (k = 0; k < potential->maxrp; k++) {
	_refine_wfb[i][k] = orb->wfun[k];
      }
    }
  } else {
    nw = 5;
    _refine_wfb = malloc(sizeof(double *)*nw);
    n2 = n+2;
    for (i = 0; i < 3; i++) {
      _refine_wfb[i] = malloc(sizeof(double)*n2);
    }
    for (i = 3; i < nw; i++) {
      _refine_wfb[i] = malloc(sizeof(double)*potential->maxrp);
    }
    _refine_wfb[0][0] = 0.0;
    xtol = 1.0/(n2-1.0);
    for (k = 1; k < n2; k++) {
      _refine_wfb[0][k] = _refine_wfb[0][k-1]+xtol;
    }
    _refine_wfb[1][0] = 0.0;
    _refine_wfb[1][n2-1] = 0.0;
    k = potential->maxrp-1;
    f = potential->Z[k] + potential->Vc[k]*potential->rad[k];
    for (k = 0; k < potential->maxrp; k++) {
      _refine_wfb[3][k] = (potential->Z[k] +
			   potential->Vc[k]*potential->rad[k])/f;
    }
  }
  _refine_mode = m;
  if (_refine_pj >= 0 && _refine_em == 1) {
    int pp, jj;
    DecodePJ(_refine_pj, &pp, &jj);
    SetSymmetry(pp, 1, &jj);
  }
  if (m > 2) {
  } else {
    _refine_msglvl = msglvl;
    xtol = _refine_xtol;
    mode = 0;
    lw = malloc(sizeof(int)*n*2);
    x = malloc(sizeof(double)*n);
    scale = malloc(sizeof(double)*n);
    dw = malloc(sizeof(double)*(n*2+n*(n+4)+1));
    f0 = 0;
    for (k = 0; k < potential->maxrp; k++) {
      f = fabs(potential->U[k]);
      if (f > f0) f0 = f;
    }
    for (i = 0; i < n; i++) {
      x[i] = 0.0;
      scale[i] = _refine_scale*f0;
    }
    f0 = EnergyFunc(&n, x);
    nfe = 0;
    ierr = 0;
    SUBPLX(EnergyFunc, n, xtol, maxfun, mode, scale, x,
	   &f, &nfe, dw, lw, &ierr);
    f = EnergyFunc(&n, x);
    double t1 = WallTime();
    if (msglvl > 0) {
      printf("refine final: ");
      for (i = 0; i < n; i++) {
	printf("%g ", x[i]);
      }
      printf("--- %d %18.10E %18.10E %d %d %10.3E\n",
	     n, f, f0, ierr, nfe, t1-t0);
    }  
    free(x);
    free(scale);
    free(lw);
    free(dw);
  }
  for (i = 0; i < nw; i++) {
    free(_refine_wfb[i]);
  }
  free(_refine_wfb);
  qed.se = se;
  CopyPotentialOMP(0);
  if (_refine_pj >= 0 && _refine_em == 1) {
    SetSymmetry(-1, 0, NULL);
  }
  return 0;
}

void Orthogonalize(ORBITAL *orb) {
  int i, k;

  for (i = 0; i < potential->maxrp; i++) {
    _yk[i] = 1;
  }
  k = ((abs(orb->kappa)-1)*2)+(orb->kappa>0);
  ORBMAP *om = &_orbmap[k];  
  ORBITAL *orb0;
  double a, b, *p, *q, *p0, *q0;
  double qn, qn0;
  Integrate(_yk, orb, orb, 1, &qn0, 0);
  p = Large(orb);
  q = Small(orb);
  b = 1.0;
  for (k = 0; k < _norbmap0; k++) {
    orb0 = om->opn[k];
    if (orb0 == NULL || orb->n == orb0->n) continue;
    if (orb0->isol <= 0) continue;
    if (_orthogonalize_mode > 1 && orb0->isol < 2) continue;
    if (orb->isol == 3 && orb0->n > orb->n) continue;
    Integrate(_yk, orb, orb0, 1, &a, 0);
    p0 = Large(orb0);
    q0 = Small(orb0);
    for (i = 0; i <= orb->ilast; i++) {
      p[i] -= p0[i]*a;
      q[i] -= q0[i]*a;
    }
  }
  Integrate(_yk, orb, orb, 1, &qn, 0);
  b = 1.0/sqrt(qn);
  for (i = 0; i <= orb->ilast; i++) {
    p[i] *= b;
    q[i] *= b;
  }
  orb->energy = ZerothHamilton(orb, orb);
}

int SolveDirac(ORBITAL *orb) {
  int err;
#ifdef PERFORM_STATISTICS
  clock_t start, stop;
  start = clock();
#endif

  err = 0;
  if (potential->npseudo > 0 &&
      orb->n < 1000000 &&
      orb->n > potential->npseudo) {
    int k = GetLFromKappa(orb->kappa)/2;
    if (orb->n > k+potential->dpseudo) {
      return 0;
    }
  }
  potential->flag = -1;
  int sb = 0;
  if (potential->sturm_idx > 0 &&
      potential->nb > 0 &&
      1+orb->energy==1 &&
      orb->n > potential->nb) {    
    if (1+_sturm_eref == 1) {
      ORBITAL *ro;
      if (_sturm_nref > 0) {
	ro = GetOrbitalSolved(OrbitalIndex(_sturm_nref, _sturm_kref, 0));
      } else {
	ro = GetOrbitalSolved(OrbitalIndex(potential->nb, _sturm_kref, 0));
      }
      _sturm_eref = ro->energy;
    }
    orb->energy = _sturm_eref;
    sb = 1;
  }
  err = RadialSolver(orb, potential);
  if (err) { 
    MPrintf(-1, "Error occured in RadialSolver: %d %d %d %18.10E\n",
	    err, orb->n, orb->kappa, orb->energy);
    average_config.n_shells = 0;
    GetPotential("error.pot", 0);
    if (err < -1 && orb->wfun != NULL) {
      orb->isol=10;
      orb->ilast = potential->maxrp-1;
      WaveFuncTableOrb("error.wav", orb);
      free(orb->wfun);
    }
    Abort(1);      
  }
  if (sb) {
    Orthogonalize(orb);
  } else {
    if (_orthogonalize_mode > 0) {
      if (potential->nfrozen > 0 && orb->n > 0) {
	Orthogonalize(orb);
      }
    }
  }
#ifdef PERFORM_STATISTICS
  stop = clock();
  rad_timing.dirac += stop - start;
#endif

  return err;
}

int WaveFuncTableOrb(char *s, ORBITAL *orb) {
  int i, j, k, n, kappa;
  FILE *f;
  double z, a, b, ke, y, e;
  
  if (orb == NULL || orb->isol == 0) return -1;
  f = fopen(s, "w");
  if (!f) return -1;
  n = orb->n;
  kappa = orb->kappa;
  k = orb->idx;
  
  fprintf(f, "#      n = %2d\n", n);
  fprintf(f, "#  kappa = %2d\n", kappa);
  fprintf(f, "# energy = %15.8E\n", orb->energy*HARTREE_EV);
  fprintf(f, "#     dn = %15.8E\n", orb->dn);
  fprintf(f, "#SelfEne = %15.8E\n", orb->se*HARTREE_EV);
  fprintf(f, "#     vc = %15.8E\n", MeanPotential(k, k)*HARTREE_EV);
  ResidualPotential(&a, k, k);  
  fprintf(f, "#     vr = %15.8E\n", a*HARTREE_EV);  
  fprintf(f, "#  ilast = %4d\n", orb->ilast);
  i = ((abs(kappa)-1)*2)+(kappa>0);
  fprintf(f, "#    ifm = %4d\n", _orbmap[i].ifm);
  fprintf(f, "#    cfm = %15.8E\n", _orbmap[i].cfm);
  fprintf(f, "#    xfm = %15.8E\n", _orbmap[i].xfm);
  fprintf(f, "#    rfn = %15.8E\n", orb->rfn);
  fprintf(f, "#    pdx = %15.8E\n", orb->pdx);
  fprintf(f, "#   bqp0 = %15.8E\n", orb->bqp0);
  fprintf(f, "#   bqp1 = %15.8E\n", orb->bqp1);
  fprintf(f, "#    idx = %d\n", k);
  PrintLepton(f);
  ORBITAL *horb = orb->horb;
  ORBITAL *rorb = orb->rorb;
  if (n != 0) {
    fprintf(f, "\n\n");
    if (n < 0) k = potential->ib;
    else k = 0;
    for (i = k; i <= orb->ilast; i++) { //Case of a bound wave function
      fprintf(f, "%-4d %14.8E %13.6E %13.6E %13.6E %13.6E", 
	      i, potential->rad[i], 
	      (potential->Vc[i])*potential->rad[i],
	      potential->U[i] * potential->rad[i],
	      Large(orb)[i], Small(orb)[i]); 
      if (rorb != NULL && rorb->wfun != NULL) {
	fprintf(f, " %13.6E %13.6E", Large(rorb)[i], Small(rorb)[i]);
      } else {
	fprintf(f, " %13.6E %13.6E", 0.0, 0.0);
      }
      if (horb != NULL && horb->wfun != NULL) {
	fprintf(f, " %13.6E %13.6E\n", Large(horb)[i], Small(horb)[i]);
      } else {
	fprintf(f, " %13.6E %13.6E\n", 0.0, 0.0);
      }
    }
  } else { //Case of a continuum wave function
    a = GetPhaseShift(k);
    while(a < 0) a += TWO_PI;
    a -= TWO_PI*((int)(a/TWO_PI));
    fprintf(f, "#  phase = %15.8E\n", a);
    fprintf(f, "\n\n");
    z = GetResidualZ();
    e = orb->energy;
    a = FINE_STRUCTURE_CONST2 * e;
    ke = sqrt(2.0*e*(1.0+0.5*a));
    y = (1.0+a)*z/ke;
    //First output region I solution to radial Dirac
    //equation (Numerov integration region)
    for (i = 0; i <= orb->ilast; i++) {
      fprintf(f, "%-4d %14.8E %13.6E %13.6E %13.6E %13.6E", 
	      i, potential->rad[i],
	      (potential->Vc[i])*potential->rad[i],
	      potential->U[i] * potential->rad[i],
	      Large(orb)[i], Small(orb)[i]);
      if (horb != NULL && horb->wfun != NULL) {
	fprintf(f, " %13.6E %13.6E\n", Large(horb)[i], Small(horb)[i]);
      } else {
	fprintf(f, " %13.6E %13.6E\n", 0.0, 0.0);
      }
    }
    //Now output region II solution to radial Dirac equation
    //(Bar-Shalom phase-amplitude approach)
    //k counts how many points are stored in the phase amplitude
    //solution (since phase and amplitude are stored in every
    //second entry)
    k = 0;
    for (; i < potential->maxrp; i += 2) {
      _dwork[k] = potential->rad[i];
      _dwork1[k] = Large(orb)[i];
      _dwork2[k] = Large(orb)[i+1];
      _dwork3[k] = Small(orb)[i];
      _dwork4[k] = Small(orb)[i+1];
      _dwork5[k] = log(_dwork[k]);
      _dwork10[k] = potential->rad[i+1];
      _dwork15[k] = log(_dwork10[k]);
      k++;
    }
    //UVIP3P performs univariate interpolation
    //First entry (3) is the degree of the polynomial used
    // in interpolation
    //Second ntry (k) is the number of data points
    //Third entry (dwork5) is the abscissas of the datapoints
    //ie. the radial coordinate
    //Fourth entry (_dwork1,2,3,4) is the input data points
    //Fifth entry is the number of desired data points (k)
    //Sixth entry (_dwork10,15) is the values of the desired 
    //data points
    //Seventh entry (_dwork11,12,13,14) is the output 

    //Interpolate amplitude of large component of orbital
    UVIP3P(3, k, _dwork5, _dwork1, k, _dwork15, _dwork11);
    //Interpolate phase     of large component of orbital
    UVIP3P(3, k, _dwork,  _dwork2, k, _dwork10, _dwork12);
    //Interpolate amplitude of small component of orbital
    UVIP3P(3, k, _dwork5, _dwork3, k, _dwork15, _dwork13);
    //Interpolate phase     of small component of orbital
    UVIP3P(3, k, _dwork5, _dwork4, k, _dwork15, _dwork14);
    double b1 = orb->kappa;
    b1 = b1*(b1+1.0) - FINE_STRUCTURE_CONST2*z*z;
    j = 0;
    for (i = orb->ilast+1; i < potential->maxrp; i += 2) {
      for (k = 0; k < 2; k++) {
	a = ke * potential->rad[i+k];
	a = a + y*log(2.0*a);
	if (k == 0) {
	  a = _dwork2[j] - a;
	} else {
	  a = _dwork12[j] - a;
	}
	a = a - ((int)(a/(TWO_PI)))*TWO_PI;
	if (a < 0) a += TWO_PI;
	b = PhaseRDependent(ke*potential->rad[i+k], y, b1);
	//Output format is now:
	//indx, r,
	//amplitude large component,
	//phase large component,
	//amplitude small component,
	//phase small component
	if (k == 0) {
	  fprintf(f, "%-4d %14.8E %13.6E %13.6E %13.6E %13.6E %13.6E %13.6E\n",
		  i, potential->rad[i+k],
		  _dwork1[j], _dwork2[j], _dwork3[j], _dwork4[j], a, b);
	} else {
	  fprintf(f, "%-4d %14.8E %13.6E %13.6E %13.6E %13.6E %13.6E %13.6E\n",
		  i+1, potential->rad[i+k],
		  _dwork11[j], _dwork12[j], _dwork13[j], _dwork14[j], a, b);
	}	
      }
      j++;
    }
  }

  fclose(f);

  return 0;
}

int WaveFuncTable(char *s, int n, int kappa, double e) {
  int k;
  ORBITAL *orb;

  //Convert electron volts to Hartree units
  e /= HARTREE_EV;
  //Map angular quantum numbers and energy to index of
  //orbital
  //If continuum wave function generate new orbital?
  k = OrbitalIndex(n, kappa, e);
  if (k < 0) return -1;
  orb = GetOrbitalSolved(k); 

  return WaveFuncTableOrb(s, orb);
}

double PhaseRDependent(double x, double eta, double b) {
  double tau, tau2, y, y2, t, a1, a2, sb;
  
  y = 1.0/x;
  y2 = y*y;
  tau2 = 1.0 + 2.0*eta*y - b*y2;
  tau = x*sqrt(tau2);
  
  t = eta*log(x+tau+eta) + tau - eta;
  if (b > 0.0) {
    sb = sqrt(b);
    a1 = b - eta*x;
    a2 = tau*sb;
    tau2 = atan2(a1, a2);
    tau2 -= atan2(-eta, sb);
    t += sb*tau2;
  } else if (b < 0.0) {
    b = -b;
    sb = sqrt(b);
    a1 = 2.0*(b+eta*x)/(sb*b*x);
    a2 = 2.0*tau/(b*x);
    tau2 = log(a1+a2);
    tau2 -= log(2.0*(eta/sb + 1.0)/b);
    t -= sb*tau2;
  }
  
  return t;
}

double GetPhaseShift(int k) {
  ORBITAL *orb;
  double ap, aa, phase1, r, y, z, ke, e, a, b1;
  int i, i0, ni;

  orb = GetOrbitalSolved(k);
  if (orb->n > 0) return 0.0;

  if (orb->phase) return *(orb->phase);

  z = GetResidualZ();
  e = orb->energy;
  a = FINE_STRUCTURE_CONST2 * e;
  ke = sqrt(2.0*e*(1.0 + 0.5*a));
  y = (1.0 + a)*z/ke;

  ap = 0.0;
  ni = 0;
  b1 = orb->kappa;
  b1 = b1*(b1+1.0) - FINE_STRUCTURE_CONST2*z*z;
  i0 = orb->ilast+2;
  i = potential->r_core+2;
  i0 = Max(i0, i);
  i = potential->maxrp-3;
  i0 = Min(i0,i);
  aa = 0.0;
  for (i = potential->maxrp-1; i > i0; i -= 2) {
    phase1 = orb->wfun[i];
    r = potential->rad[i-1];      
    a = ke * r;
    phase1 = phase1 - PhaseRDependent(a, y, b1);
    if (ni > 3) {
      if (ni > 25) break;
      if (fabs(phase1-aa) > 0.01*fabs(aa)) break;
    }
    ap += phase1;
    ni++;
    aa = ap/ni;
  }
  orb->phase = malloc(sizeof(double));
  if (aa < 0 || aa > TWO_PI) aa -= TWO_PI*floor(aa/TWO_PI);
  *(orb->phase) = aa;

  return aa;  
}

int GetNumBounds(void) {
  return n_orbitals - n_continua;
}

int GetNumOrbitals(void) {
  return n_orbitals;
}

int GetNumContinua(void) {
  return n_continua;
}

int OrbitalIndex(int n, int kappa, double energy) {
  ORBITAL *orb = NULL;
  ORBITAL **porb;
  int idx[3];
  
  int k;
  if (n == 0) {
    k = 0;
  } else {
    k = ((abs(kappa)-1)*2)+(kappa>0);  
    if (k >= _korbmap) {
      printf("too large kappa, enlarge korbmap: %d >= %d\n",
	     k, _korbmap);
      Abort(1);
    }
  }
  ORBMAP *om = &_orbmap[k];
  if (n > 0) { //Case of a bound orbital
    k = n-1;
    if (k >= _norbmap0) {
      printf("too many bound orbitals, enlarge norbmap0: %d >= %d\n",
	     k, _norbmap0);
      Abort(1);
    }
    porb = &om->opn[k];
    if (potential->ib == 0 && n > potential->nmax) {
      if (om->ifm == 0) {
	int km = OrbitalIndex(potential->nmax, kappa, energy);
	ORBITAL *omn = GetOrbital(km);
	double *pp = Large(omn);
	double *qq = Small(omn);
	om->ifm = FirstMaximum(pp, potential->r_core,
			      potential->maxrp-1, potential);
	int i;
	for (i = 0; i <= om->ifm; i++) {
	  potential->dW2[i] = (pp[i]*pp[i] + qq[i]*qq[i])*potential->dr_drho[i];
	}
	om->cfm = Simpson(potential->dW2, 0, om->ifm);
	
	km = OrbitalIndex(potential->nmax-2, kappa, energy);
	omn = GetOrbital(km);
	pp = Large(omn);
	qq = Small(omn);
	for (i = 0; i <= om->ifm; i++) {
	  potential->dW2[i] = (pp[i]*pp[i] + qq[i]*qq[i])*potential->dr_drho[i];
	}
	om->xfm = Simpson(potential->dW2, 0, om->ifm);
	om->xfm = log(om->cfm/om->xfm);
	om->xfm /= log((potential->nmax-2.0)/potential->nmax);
	if (om->xfm > 3.0) om->xfm = 3.0;
      }
      potential->ifm = om->ifm;
      potential->cfm = om->cfm;
      potential->xfm = om->xfm;
    }
  } else if (n < 0) {
    k = -n-1;
    porb = &om->onn[k];
  } else {
    SetEnergyIndex(idx, kappa, energy);
    porb = (ORBITAL **) MultiSet(om->ozn, idx, NULL, NULL,
				 InitPointerData, NULL);
  }
#pragma omp atomic read
  orb = *porb;    

  if (orb == NULL) {
    if (orbitals->lock) {
      SetLock(orbitals->lock);
#pragma omp atomic read
      orb = *porb;
    }
    if (orb == NULL) {
      orb = GetNewOrbitalNoLock(n, kappa, energy, porb);
    }
    if (orbitals->lock) ReleaseLock(orbitals->lock);
  } else if (orb->isol == 0) {
    if (orbitals->lock) SetLock(orbitals->lock);
    if (orb->isol == 0) {
      k = SolveDirac(orb);
      if (k < 0) {
	MPrintf(-1, "Error occured in solving Dirac eq. err = %d\n", k);
	Abort(1);
      }
    }
    if (orbitals->lock) ReleaseLock(orbitals->lock);
  }
  if (potential->npseudo > 0 && orb->n > potential->npseudo) {
    k = GetLFromKappa(orb->kappa)/2;
    if (orb->n > k+potential->dpseudo) {
      return orb->idx;
    }
  }
  if (!orb->isol) {
    printf("isol0a: %d %d %d %g\n", orb->idx, orb->n, orb->kappa, orb->energy);
    Abort(1);
  }
  return orb->idx;
}

int OrbitalExistsNoLock(int n, int kappa, double energy) {
  ORBITAL *orb = NULL;
  ORBITAL **porb;
  int idx[3];
  
  int k;
  if (n == 0) {
    k = 0;
  } else {
    k = ((abs(kappa)-1)<<1)+(kappa>0);  
    if (k >= _korbmap) {
      printf("too large kappa, enlarge korbmap: %d >= %d\n",
	     k, _korbmap);
      Abort(1);
    }
  }
  ORBMAP *om = &_orbmap[k];
  if (n > 0) {
    k = n-1;
#pragma omp atomic read
    orb = om->opn[k];
  } else if (n < 0) {
    k = -n-1;
#pragma omp atomic read
    orb = om->onn[k];
  } else {
    SetEnergyIndex(idx, kappa, energy);
    porb = (ORBITAL **) MultiGet(om->ozn, idx, NULL);
    if (porb) orb = *porb;
  }
  if (orb != NULL) {
    return orb->idx;
  }
  return -1;
}

int OrbitalExists(int n, int kappa, double energy) {
  int i;
  i = OrbitalExistsNoLock(n, kappa, energy);
  if (i >= 0) return i;
  if (orbitals->lock) {
    SetLock(orbitals->lock);
    i = OrbitalExistsNoLock(n, kappa, energy);
    ReleaseLock(orbitals->lock);
  }
  return i;
}

void SetEnergyIndex(int idx[3], int kappa, double energy) {
  char *p;
  int k, i, ip[8];
  
  p = (char *) (&energy);
  for (i = 0; i < 8; i++) {
    ip[i] = p[i];
    if (ip[i] < 0) ip[i] = 128+(-ip[i]-1);
  }
  k = ((abs(kappa)-1)*2)+(kappa>0);
  idx[0] = k*65536 + ip[0]*256 + ip[1];
  idx[1] = ip[2]*65536 + ip[3]*256 + ip[4];
  idx[2] = ip[5]*65536 + ip[6]*256 + ip[7];
  //printf("%d %d %d %d %d %d %d %d %d %d %d %d\n", k, ip[0],ip[1],ip[2],ip[3],ip[4],ip[5],ip[6],ip[7],idx[0],idx[1],idx[2]);
}

void AddOrbMap(ORBITAL *orb) {
  int k;
  if (orb->n == 0) {
    k = 0;
  } else {
    k = ((abs(orb->kappa)-1)<<1)+(orb->kappa>0);
    if (k >= _korbmap) {
      printf("too large kappa, enlarge korbmap: %d >= %d\n",
	     k, _korbmap);
      Abort(1);
    }
  }
  ORBMAP *om = &_orbmap[k];  
  if (orb->n > 0) {
    k = orb->n-1;
    if (k >= _norbmap0) {
      printf("too many bound orbitals, enlarge norbmap0: %d >= %d\n",
	     k, _norbmap0);
      Abort(1);
    }
#pragma omp atomic write
    om->opn[k] = orb;
  } else if (orb->n < 0) {
    k = -orb->n-1;
    if (k >= _norbmap1) {
      printf("too many basis orbitals, enlarge norbmap1: %d >= %d\n",
	     k, _norbmap1);
      Abort(1);
    }
#pragma omp atomic write
    om->onn[k] = orb;
  } else {
    int idx[3];
    SetEnergyIndex(idx, orb->kappa, orb->energy);
    MultiSet(om->ozn, idx, &orb, NULL, InitPointerData, NULL);
  }
#pragma omp flush
}

void RemoveOrbMap(int m) {
  int k, i;
  int blocks[3] = {MULTI_BLOCK3, MULTI_BLOCK3, MULTI_BLOCK3};
  for (k = 0; k < _korbmap; k++) {
    ORBMAP *om = &_orbmap[k];
    if (m == 0) {
      for (i = 0; i < _norbmap0; i++) {
	om->opn[i] = NULL;
      }
    }
    for (i = 0; i < _norbmap1; i++) {
      om->onn[i] = NULL;
    }
    if (k == 0) {
      MultiFreeData(om->ozn, NULL);
      //MultiInit(om->ozn, sizeof(ORBITAL *), 3, blocks, "ozn");
    }
  }
}

ORBITAL *GetOrbital(int k) {
  return (ORBITAL *) ArrayGet(orbitals, k);
}

ORBITAL *GetOrbitalSolved(int k) {
  ORBITAL *orb;
  int i;
  //Attempt to retrieve solved orbital from memory
  
  orb = (ORBITAL *) ArrayGet(orbitals, k);
  //If orbital exists and has been solved return
  //the orb object
  if (orb != NULL && orb->isol) return orb;  
  if (orbitals->lock) {
    SetLock(orbitals->lock);
    orb = (ORBITAL *) ArrayGet(orbitals, k);
  }
  if (orb->isol == 0) {
    MPrintf(-1, "isol0b: %d %d %d %d %g %x\n",
	    k, orbitals->dim, orb->n, orb->kappa, orb->energy, orbitals->lock);
    Abort(1);
    i = SolveDirac(orb);
    if (i < 0) {
      printf("Error occured in solving Dirac eq. err = %d\n", i);
      Abort(1);
    }
  }
  if (orbitals->lock) ReleaseLock(orbitals->lock);
  return orb;
}

ORBITAL *GetOrbitalSolvedNoLock(int k) {
  ORBITAL *orb;
  int i;
  
  orb = (ORBITAL *) ArrayGet(orbitals, k);
  if (orb != NULL && orb->isol) return orb;
  
  orb = (ORBITAL *) ArrayGet(orbitals, k);
  if (orb->isol == 0) {
    i = SolveDirac(orb);
    if (i < 0) {
      printf("Error occured in solving Dirac eq. err = %d\n", i);
      Abort(1);
    }
  }
  return orb;
}

ORBITAL *GetNewOrbitalNoLock(int n, int kappa, double e, ORBITAL **solve) {
  ORBITAL *orb;
  //Add new orbital to global list
  orb = (ORBITAL *) ArrayAppend(orbitals, NULL, InitOrbitalData);
  if (!orb) {
    printf("Not enough memory for orbitals array\n");
    Abort(1);
  }
  orb->idx = n_orbitals;
  orb->n = n;
  orb->kappa = kappa;
  orb->energy = e;
  orb->isol = 0;
  n_orbitals++;
  if (n == 0) {
    n_continua++;
  }
  if (solve) {
    int k = SolveDirac(orb);
    if (k < 0) {
      MPrintf(-1, "Error occured in solving Dirac eq. err = %d\n", k);
      Abort(1);
    }
#pragma omp atomic write
    *solve = orb;
  } else {    
    AddOrbMap(orb);
  }
#pragma omp flush
  return orb;
}

ORBITAL *GetNewOrbital(int n, int kappa, double e) {
  ORBITAL *orb;

  if (orbitals->lock) SetLock(orbitals->lock);
  orb = GetNewOrbitalNoLock(n, kappa, e, NULL);
  if (orbitals->lock) ReleaseLock(orbitals->lock);
  return orb;
}

void FreeOrbitalData(void *p) {
  ORBITAL *orb;

  orb = (ORBITAL *) p;
  //RemoveOrbMap(orb);
  if (orb->wfun) {
    free(orb->wfun);
  }
  if (orb->phase) free(orb->phase);
  orb->wfun = NULL;
  orb->phase = NULL;
  orb->isol = 0;
  orb->ilast = -1;
  //orb->im = -1;
  if (orb->horb && orb->horb != orb) {
    FreeOrbitalData(orb->horb);
    free(orb->horb);
  }
  orb->horb = NULL;
  if (orb->rorb && orb->rorb != orb) {
    FreeOrbitalData(orb->rorb);
    free(orb->rorb);
  }
  orb->rorb = NULL;
}

int ClearOrbitalTable(int m) {
  ORBITAL *orb;
  int i;

  if (orbitals->lock) SetLock(orbitals->lock);
  if (m == 0) {
    n_orbitals = 0;
    n_continua = 0;
    ArrayFree(orbitals, FreeOrbitalData);
    RemoveOrbMap(0);
  } else {
    for (i = n_orbitals-1; i >= 0; i--) {
      orb = GetOrbital(i);
      if (orb->n == 0) {
	n_continua--;
      }
      if (orb->n > 0) {
	n_orbitals = i+1;
	ArrayTrim(orbitals, i+1, FreeOrbitalData);
	RemoveOrbMap(1);
	break;
      }
    }
  }
  if (orbitals->lock) ReleaseLock(orbitals->lock);
  return 0;
}

int SkipOptGrp(CONFIG_GROUP *g) {
  int skip, nmax, i, ic;
  CONFIG *cfg;
  
  nmax = potential->nmax-1;
  if (potential->nb > 0 && nmax < potential->nb) nmax = potential->nb;
  skip = nmax>0 && g->nmax>nmax;
  if (!skip) {
    nmax = GetOrbNMax(-1, 0);
    if (nmax > 0) {
      for (i = 0; i < g->n_cfgs; i++) {
	cfg = (CONFIG *) ArrayGet(&(g->cfg_list), i);
	for (ic = 0; ic < cfg->n_shells; ic++) {
	  nmax = GetOrbNMax(cfg->shells[ic].kappa, 0);
	  if (nmax > 0 && cfg->shells[ic].n >= nmax) {
	    skip = 1;
	    break;
	  }
	}
	if (skip) break;
      }	    
    }
  }
  return skip;
} 

int ConfigEnergy(int m, int mr, int ng, int *kg) {
  CONFIG_GROUP *g;
  CONFIG *cfg;
  int k, kk, i, md, md1, csi;
  int nog, iog, **og, *gp, ngp;
  double e0;
  char *sid, pfn[2048];

  if (optimize_control.mce < 0) return 0;
  if (kg == NULL) ng = 0;
  else if (ng <= 0) kg = NULL;
  if (ng == 0) {
    ng = GetNumGroups();
  }
  og = GetOptGrps(&nog);
  if (!og) {
    nog = ng;
  }  
  md = optimize_control.mce%10;  
  md1 = 10*(optimize_control.mce/10);
  if (m == 0) {
    csi = _csi;
    if (fabs(_config_energy_csi) < 100.0) {
      _csi = _config_energy_csi;
    }
    for (iog = 0; iog < nog; iog++) {
      if (og) {
	gp = malloc(sizeof(int)*og[iog][0]);
	ngp = 0;
	for (k = 0; k < og[iog][0]; k++) {
	  g = GetGroup(og[iog][k+1]);
	  for (i = 0; i < g->n_cfgs; i++) {
	    cfg = (CONFIG *) ArrayGet(&(g->cfg_list), i);
	    cfg->energy = 0;
	    cfg->delta = 0;
	  }
	  if (SkipOptGrp(g)) continue;
	  gp[ngp++] = og[iog][k+1];
	}
	sid = GetOptGrpId(iog);
      } else {
	ngp = 1;
	if (kg) kk = kg[iog];
	else kk = iog;
	gp = &kk;
	g = GetGroup(kk);
	for (i = 0; i < g->n_cfgs; i++) {
	  cfg = (CONFIG *) ArrayGet(&(g->cfg_list), i);
	  cfg->energy = 0;
	  cfg->delta = 0;
	}
	if (SkipOptGrp(g)) {
	  ngp = 0;
	}
	sid = g->name;
      }
      if (ngp == 0) {
	continue;
      }
      if (md == 0) {
	if (_config_energy_psr == 2) {
	  sprintf(pfn, "%s%s.pot", _config_energy_pfn, sid);
	  if (RestorePotential(pfn, potential) < 0) {
	    ReinitRadial(1);
	    ClearOrbitalTable(0);
	    continue;
	  }
	} else {
	  k = OptimizeRadial(ngp, gp, -1, NULL, 1);
	  if (k < 0) {
	    ReinitRadial(1);
	    ClearOrbitalTable(0);
	    continue;
	  }
	  if (mr > 0) RefineRadial(mr, 0, 0, -1);
	  if (_config_energy_psr == 1) {
	    sprintf(pfn, "%s%s.pot", _config_energy_pfn, sid);
	    SavePotential(pfn, potential);
	  }
	}
      }
      for (k = 0; k < ngp; k++) {	
	g = GetGroup(gp[k]);
	for (i = 0; i < g->n_cfgs; i++) {
	  if (md > 0) {
	    if (_config_energy_psr == 2) {
	      sprintf(pfn, "%s%s%d.pot", _config_energy_pfn, g->name, i);
	      if (RestorePotential(pfn, potential) < 0) {
		ReinitRadial(1);
		ClearOrbitalTable(0);
		continue;
	      }
	    } else {
	      if (OptimizeRadial(1, gp+k, i, NULL, 1) < 0) {
		ReinitRadial(1);
		ClearOrbitalTable(0);
		continue;
	      }
	      if (mr > 0) RefineRadial(mr, 0, 0, -1);
	      if (_config_energy_psr == 1) {
		sprintf(pfn, "%s%s%d.pot", _config_energy_pfn, g->name, i);
		SavePotential(pfn, potential);
	      }
	    }
	  }
	  cfg = (CONFIG *) ArrayGet(&(g->cfg_list), i);
	  e0 = AverageEnergyConfigMode(cfg, md1);
	  cfg->energy = e0;
	  if (md > 0) {
	    ReinitRadial(1);
	    ClearOrbitalTable(0);
	  }
	}
      }
      if (md == 0) {
	ReinitRadial(1);
	ClearOrbitalTable(0);
      }
      if (og) free(gp);      
    }
    if (fabs(_config_energy_csi) < 100) {
      _csi = csi;
    }
  } else if (md1 < 20) {
    for (iog = 0; iog < nog; iog++) {
      if (og) {
	ngp = og[iog][0];
	gp = og[iog]+1;
      } else {
	ngp = 1;
	if (kg) kk = kg[iog];
	else kk = iog;
	gp = &kk;
      }
      for (k = 0; k < ngp; k++) {
	kk = gp[k];
	g = GetGroup(kk);
	for (i = 0; i < g->n_cfgs; i++) {
	  cfg = (CONFIG *) ArrayGet(&(g->cfg_list), i);
	  if (cfg->energy != 0) {
	    e0 = AverageEnergyConfigMode(cfg, md1);
	    cfg->delta = cfg->energy - e0;	
	    if (optimize_control.iprint) {
	      MPrintf(-1, "ConfigEnergy: %d %d %d %d %g %g %g %s\n", 
		      md, md1, kk, i, cfg->energy, e0, cfg->delta, g->name);
	    }
	  }
	}
      }
    }
  }
  return 0;
}

double TotalEnergyGroup(int kg) {
  return TotalEnergyGroupMode(kg, 0);
}

/* calculate the total configuration average energy of a group. */
double TotalEnergyGroupMode(int kg, int md) {
  CONFIG_GROUP *g;
  ARRAY *c;
  CONFIG *cfg;
  int t;
  double total_energy, a, w;

  g = GetGroup(kg);
  c = &(g->cfg_list);
  
  total_energy = 0.0;
  g->sweight = 0.0;
  for (t = 0; t < g->n_cfgs; t++) {
    cfg = (CONFIG *) ArrayGet(c, t);
    a = AverageEnergyConfigMode(cfg, md);
    w = fabs(cfg->sweight);
    total_energy += a*w;
    g->sweight += w;
  }
  total_energy /= g->sweight;
  return total_energy;
}

double TotalEnergyGroups(int ng, int *kg, double *w) {
  double a, r, b;
  CONFIG_GROUP *g;
  int i;
  r = 0.0;
  b = 0.0;
  for (i = 0; i < ng; i++) {
    a = TotalEnergyGroup(kg[i]);
    if (w) {
      r += w[i]*a;
      b += w[i];
    } else {
      g = GetGroup(kg[i]);
      r += a*g->sweight;
      b += g->sweight;
    }
  }
  if (b > 0) r /= b;
  return r;
}

double ZerothEnergyConfig(CONFIG *cfg) {
  int i, n, nq, kappa, k;
  double r, e;

  r = 0.0;
  for (i = 0; i < cfg->n_shells; i++) {
    n = (cfg->shells[i]).n;
    nq = (cfg->shells[i]).nq;
    kappa = (cfg->shells[i]).kappa;
    k = OrbitalIndex(n, kappa, 0.0);
    e = GetOrbital(k)->energy;
    r += nq * e;
  }
  return r;
}

double ZerothResidualConfig(CONFIG *cfg) {
  int i, n, nq, kappa, k;
  double r, e;

  r = 0.0;
  for (i = 0; i < cfg->n_shells; i++) {
    n = (cfg->shells[i]).n;
    nq = (cfg->shells[i]).nq;
    kappa = (cfg->shells[i]).kappa;
    k = OrbitalIndex(n, kappa, 0.0);
    ResidualPotential(&e, k, k);
    r += nq * e;
  }
  return r;
}
  
static double FKB(int ka, int kb, int k) {
  int ja, jb, ia, ib;
  double a, b;
  
  GetJLFromKappa(GetOrbital(ka)->kappa, &ja, &ia);
  GetJLFromKappa(GetOrbital(kb)->kappa, &jb, &ib);

  if (!Triangle(ia, k, ia) || !Triangle(ib, k, ib)) return 0.0;
  a = W3j(ja, k, ja, 1, 0, -1)*W3j(jb, k, jb, 1, 0, -1);
  if (fabs(a) < EPS30) return 0.0; 
  Slater(&b, ka, kb, ka, kb, k/2, 0);
  
  b *= a*(ja+1.0)*(jb+1.0);
  if (IsEven((ja+jb)/2)) b = -b;

  return b;
}

static double GKB(int ka, int kb, int k) {
  int ja, jb, ia, ib;
  double a, b;
  
  GetJLFromKappa(GetOrbital(ka)->kappa, &ja, &ia);
  GetJLFromKappa(GetOrbital(kb)->kappa, &jb, &ib);
  
  if (IsOdd((ia+k+ib)/2) || !Triangle(ia, k, ib)) return 0.0;
  a = W3j(ja, k, jb, 1, 0, -1);
  if (fabs(a) < EPS30) return 0.0;
  Slater(&b, ka, kb, kb, ka, k/2, 0);
  
  b *= a*a*(ja+1.0)*(jb+1.0);
  if (IsEven((ja+jb)/2)) b = -b;
  return b;
}  
  
static double ConfigEnergyVarianceParts0(SHELL *bra, int ia, int ib, 
					 int m2, int p) {
  int ja, jb, k, kp, k0, k1, kp0, kp1, ka, kb;
  double a, b, c, d, e;

  ja = GetJFromKappa(bra[ia].kappa);
  ka = OrbitalIndex(bra[ia].n, bra[ia].kappa, 0);
  if (p > 0) {
    jb = GetJFromKappa(bra[ib].kappa);
    kb = OrbitalIndex(bra[ib].n, bra[ib].kappa, 0);
  }
  e = 0.0;
  switch (p) {
  case 0:
    k0 = 4;
    k1 = 2*ja;
    a = 1.0/(ja*(ja+1.0));
    for (k = k0; k <= k1; k += 4) {
      for (kp = k0; kp <= k1; kp += 4) {
	b = -a + W6j(ja, ja, k, ja, ja, kp);
	if (k == kp) b += 1.0/(k+1.0);
	b *= a*FKB(ka, ka, k)*FKB(ka, ka, kp);
	e += b;
      }
    }
    break;
  case 1:
    k0 = 4;
    k1 = 2*ja;
    kp0 = 4;
    kp1 = 2*jb;
    kp1 = Min(kp1, k1);
    a = 1.0/(ja*(ja+1.0));
    for (k = k0; k <= k1; k += 4) {
      for (kp = kp0; kp <= kp1; kp += 4) {
	b = a - W6j(ja, ja, kp, ja, ja, k);
	if (k == kp) b -= 1.0/(k+1.0);
	b *= W6j(ja, ja, kp, jb, jb, m2);
	b /= 0.5*ja;
	b *= FKB(ka, ka, k)*FKB(ka, kb, kp);
	e += b;
      }
    }
    if (IsOdd((ja+jb+m2)/2)) e = -e;
    break;
  case 2:
    k0 = 4;
    k1 = 2*ja;
    kp0 = abs(ja-jb);
    kp1 = ja + jb;
    a = 1.0/(ja*(ja+1.0));
    for (k = k0; k <= k1; k += 4) {
      for (kp = kp0; kp <= kp1; kp += 2) {
	b = W6j(k, kp, m2, jb, ja, ja);
	b = -b*b;
	d=W6j(jb, jb, k, ja, ja, m2)*W6j(jb, jb, k, ja, ja, kp);
	if(IsOdd((m2+kp)/2)) b -= d;
	else b += d;
	if (m2 == kp) {
	  b -= (1.0/(m2+1.0)-1.0/(jb+1.0))*a;
	} else {
	  b += a/(jb+1.0);
	}
	b /= 0.5*ja;
	b *= FKB(ka, ka, k)*GKB(ka, kb, kp);
	e += b;
      }
    }
    if (IsOdd((ja+jb)/2+1)) e = -e;
    break;
  case 3:
    k0 = 4;
    k1 = 2*ja;
    k = 2*jb;
    k1 = Min(k, k1);
    for (k = k0; k <= k1; k += 4) {
      for (kp = k0; kp <= k1; kp += 4) {
	b = 0.0;
	if (k == kp) b += 1.0/((k+1.0)*(jb+1.0));
	b -= W9j(ja, ja, k, ja, m2, jb, kp, jb, jb);
	b -= W6j(ja, jb, m2, jb, ja, k)*W6j(ja, jb, m2, jb, ja, kp)/ja;
	b /= ja;
	b *= FKB(ka, kb, k)*FKB(ka, kb, kp);
	e += b;
      }
    }
    break;
  case 4:
    k0 = abs(ja-jb);
    k1 = ja+jb;
    for (k = k0; k <= k1; k += 2) {
      for (kp = k0; kp <= k1; kp += 2) {
	b = 0.0;
	if (k == kp) b += 1.0/((k+1.0)*(jb+1.0));
	b -= W9j(ja, jb, k, jb, m2, ja, kp, ja, jb);
	c = -1.0/(jb+1.0);	
	d = c;
	if (k == m2) {
	  c += 1.0/(m2+1.0);
	}
	if (kp == m2) {	  
	  d += 1.0/(m2+1.0);
	}
	b -= c*d/ja;
	b /= ja;
	b *= GKB(ka, kb, k)*GKB(ka, kb, kp);
	e += b;
      }
    }
    break;
  case 5:
    k0 = 4;
    k1 = 2*ja;
    k = 2*jb;
    k1 = Min(k1, k);
    kp0 = abs(ja-jb);
    kp1 = ja+jb;
    for (k = k0; k <= k1; k += 4) {
      for (kp = kp0; kp <= kp1; kp += 2) {
      	b=W6j(jb, jb, k, ja, ja, kp)/(jb+1.0);
      	if(IsOdd(kp/2)) b=-b;
	c = W6j(k, kp, m2, ja, jb, jb)*W6j(k, kp, m2, jb, ja, ja);
	if (IsOdd((ja+jb+kp+m2)/2)) b += c;
	else b -= c;
	c = -1.0/(jb+1.0);
	if (kp == m2) c += 1.0/(m2+1.0);
	c *= W6j(ja, jb, m2, jb, ja, k)/ja;
	if(IsOdd(m2/2)) b += c;
	else b -= c;
	b /= 0.5*ja;
	b *= FKB(ka, kb, k)*GKB(ka, kb, kp);
	e += b;
      }
    }
    break;
  case 6:
    k0 = 4;
    k1 = 2*ja;
    k = 2*jb;
    k1 = Min(k, k1);
    a = 1.0/((ja+1.0)*(jb+1.0));
    for (k = k0; k <= k1; k += 4) {
      b = a/(k+1.0);
      c = FKB(ka, kb, k);
      b *= c*c;
      e += b;
    }
    break;
  case 7:
    k0 = abs(ja-jb);
    k1 = ja+jb;
    a = 1.0/((ja+1.0)*(jb+1.0));
    for (k = k0; k <= k1; k += 2) {
      for (kp = k0; kp <= k1; kp += 2) {
	b = 0;
	if (k == kp) {
	  b += 1.0/(k+1.0);
	}
	b -= a;
	c = GKB(ka, kb, k);
	d = GKB(ka, kb, kp);
	e += a*b*c*d;
      }
    }
    break;
  case 8:    
    k0 = 4;
    k1 = 2*ja;
    k = 2*jb;
    k1 = Min(k, k1);
    kp0 = abs(ja-jb);
    kp1 = ja+jb;
    a = 1.0/((ja+1.0)*(jb+1.0));
    for (k = k0; k <= k1; k += 4) {
      for (kp = kp0; kp <= kp1; kp += 2) {
	b = W6j(jb, ja, kp, ja, jb, k);
	if (fabs(b) < EPS30) continue;
	b *= 2.0*a;
	if (IsOdd(kp/2)) b = -b;
	c = FKB(ka, kb, k);
	d = GKB(ka, kb, kp);
	e += b*c*d;
      }
    }
    break;
  }

  return e;
}

static double ConfigEnergyVarianceParts1(SHELL *bra, int i, 
					 int ia, int ib, int m2, int p) {  
  int js, ja, jb, k, kp, k0, k1, kp0, kp1, ka, kb, ks;
  double a, b, e;
  
  js = GetJFromKappa(bra[i].kappa);
  ks = OrbitalIndex(bra[i].n, bra[i].kappa, 0);
  ja = GetJFromKappa(bra[ia].kappa);
  ka = OrbitalIndex(bra[ia].n, bra[ia].kappa, 0);
  jb = GetJFromKappa(bra[ib].kappa);
  kb = OrbitalIndex(bra[ib].n, bra[ib].kappa, 0);
  e = 0.0;

  switch (p) {
  case 0:
    k0 = 4;
    k1 = 2*ja;
    k = 2*jb;
    k1 = Min(k, k1);
    k = 2*js;
    k1 = Min(k, k1);
    for (k = k0; k <= k1; k += 4) {
      b = W6j(ja, ja, k, jb, jb, m2);
      if (fabs(b) < EPS30) continue;
      b *= 2.0/((k+1.0)*(js+1.0));
      b *= FKB(ks, ka, k)*FKB(ks, kb, k);
      if (IsEven((ja+jb+m2)/2)) b = -b;
      e += b;
    }
    break;
  case 1:
    k0 = abs(js-ja);
    k1 = js+ja;
    kp0 = abs(js-jb);
    kp1 = js+jb;
    a = 1.0/((js+1.0)*(ja+1.0)*(jb+1.0));
    for (k = k0; k <= k1; k += 2) {
      for (kp = kp0; kp <= kp1; kp += 2) {
	b = W6j(k, kp, m2, jb, ja, js);
	b = -b*b + a;
	b /= (js+1.0);
	b *= 2.0*GKB(ks, ka, k)*GKB(ks, kb, kp);
	if (IsOdd((ja+jb)/2+1)) b = -b;
	e += b;
      }
    }
    break;
  case 2:
    k0 = 4;
    k1 = 2*js;    
    k = 2*ja;
    k1 = Min(k, k1);
    kp0 = abs(js-jb);
    kp1 = js+jb;
    for (k = k0; k <= k1; k += 4) {
      for (kp = kp0; kp <= kp1; kp += 2) {
	b = W6j(jb, jb, k, ja, ja, m2);
	b *= W6j(jb, jb, k, js, js, kp);
	if (fabs(b) < EPS30) continue;
	b /= (js+1.0);
	if (IsEven((ja+jb+kp+m2)/2)) b = -b;
	b *= 2.0*FKB(ks, ka, k)*GKB(ks, kb, kp);
	e += b;
      }
    }
    break;
  }

  return e;
}

double ConfigEnergyVariance(int ns, SHELL *bra, int ia, int ib, int m2) {
  int i, js, p;
  double e, a, b, c;
  
  e = 0.0;
  for (i = 0; i < ns; i++) {
    js = GetJFromKappa(bra[i].kappa);
    a = bra[i].nq;
    b = js+1.0 - bra[i].nq;
    if (i == ia) {
      a -= 1.0;      
    }
    if (i == ib) {
      b -= 1.0;
    }
    if (a == 0.0 || b == 0.0) continue;
    a = a*b;
    b = 0.0;
    if (i == ia) {
      for (p = 0; p < 6; p++) {
	c = ConfigEnergyVarianceParts0(bra, ia, ib, m2, p);
	b += c;
      }
      b /= js-1.0;
    } else if (i == ib) {
      for (p = 0; p < 6; p++) {
	c = ConfigEnergyVarianceParts0(bra, ib, ia, m2, p);
	b += c;
      }
      b /= js-1.0;
    } else {
      for (p = 6; p < 9; p++) {
	c = ConfigEnergyVarianceParts0(bra, i, ia, m2, p);
	b += c;
	c = ConfigEnergyVarianceParts0(bra, i, ib, m2, p);
	b += c;
      }
      c = ConfigEnergyVarianceParts1(bra, i, ia, ib, m2, 0);
      b += c;
      c = ConfigEnergyVarianceParts1(bra, i, ia, ib, m2, 1);
      b += c;
      c = ConfigEnergyVarianceParts1(bra, i, ia, ib, m2, 2);
      b += c;
      c = ConfigEnergyVarianceParts1(bra, i, ib, ia, m2, 2);
      b += c;
      b /= js;
    }

    e += a*b;
  }
    
  if (e < 0.0) e = 0.0;
  return e;
}

double ConfigEnergyShiftCI(int nrs0, int nrs1) {
  int n0, k0, q0, n1, k1, q1;
  int j0, j1, s0, s1, kmax, k;
  double a, g, pk, w, sd, q;

  UnpackNRShell(&nrs0, &n0, &k0, &q0);
  UnpackNRShell(&nrs1, &n1, &k1, &q1);
  q = (q0-1.0)/(2.0*k0+1.0) - q1/(2.0*k1+1.0);
  if (q == 0.0) return q;
  g = 0.0;
  for (j0 = k0-1; j0 <= k0+1; j0 += 2) {
    if (j0 < 0) continue;
    s0 = OrbitalIndex(n0, GetKappaFromJL(j0, k0), 0);    
    for (j1 = k1-1; j1 <= k1+1; j1 += 2) {
      if (j1 < 0) continue;
      s1 = OrbitalIndex(n1, GetKappaFromJL(j1, k1), 0);
      w = W6j(j0, k0, 1, k1, j1, 2);
      w = w*w*(j0+1.0)*(j1+1.0)*0.5;
      pk = ((j0+1.0)*(j1+1.0))/((k0+1.0)*(k1+1.0)*4.0) - w;
      Slater(&sd, s0, s1, s0, s1, 0, 0);
      g += sd*pk;
      kmax = 2*j0;
      k = 2*j1;
      kmax = Min(k, kmax);
      for (k = 4; k <= kmax; k += 4) {	
	pk = W3j(k0, k, k0, 0, 0, 0);
	if (fabs(pk) > 0) {
	  pk *= W3j(k1, k, k1, 0, 0, 0);
	  if (fabs(pk) > 0) {
	    pk *= 0.25;
	    a = W6j(j0, 2, j1, k1, 1, k0);
	    if (fabs(a) > 0) {
	      a = a*a;
	      a *= W6j(j0, k, j0, k0, 1, k0);
	      if (fabs(a) > 0) {
		a *= W6j(j1, k, j1, k1, 1, k1);
		if (fabs(a) > 0) {
		  a *= W6j(j0, k, j0, j1, 2, j1);
		  a *= 2.0*(j0+1.0)*(j1+1.0)*(k0+1.0)*(k1+1.0);
		}
	      }
	    }
	    a += W6j(k0, k, k0, k1, 2, k1);
	    pk *= a;
	  }
	  if (fabs(pk) > 0) {
	    Slater(&sd, s0, s1, s0, s1, k/2, 0);
	    g += pk*sd*(j0+1.0)*(j1+1.0);
	  }
	}
      }
      pk = W3j(k0, 2, k1, 0, 0, 0);
      if (fabs(pk) > 0) {
	pk = pk*pk;
	a = W6j(j0, 2, j1, k1, 1, k0);
	a *= a;
	pk *= a;
	pk *= (k0+1.0)*(k1+1.0)/3.0;
	pk *= (1.0 - 0.5*(j0+1.0)*(j1+1.0)*a);
	if (fabs(pk) > 0) {
	  Slater(&sd, s0, s1, s1, s0, 1, 0);
	  g += pk*sd*(j0+1.0)*(j1+1.0);
	}
      }
    }
  }

  return q*g;
}
	  
double ConfigEnergyShift(int ns, SHELL *bra, int ia, int ib, int m2) {
  double qa, qb, a, b, c, sd, e;
  int ja, jb, k, kmin, kmax;
  int k0, k1;

  qa = bra[ia].nq;
  qb = bra[ib].nq;
  ja = GetJFromKappa(bra[ia].kappa);
  jb = GetJFromKappa(bra[ib].kappa);
  if (qa == 1 && qb == 0) e = 0.0;
  else {
    e = (qa-1.0)/ja - qb/jb;
    if (e != 0.0) {
      kmin = 4;
      kmax = 2*ja;
      k = 2*jb;
      kmax = Min(kmax, k);
      a = 0.0;
      k0 = OrbitalIndex(bra[ia].n, bra[ia].kappa, 0);
      k1 = OrbitalIndex(bra[ib].n, bra[ib].kappa, 0);
      for (k = kmin; k <= kmax; k += 4) {
	b = W6j(k, ja, ja, m2, jb, jb);	
	if (fabs(b) > EPS30) {
	  a += b*FKB(k0, k1, k);
	}
      }
      
      if(IsOdd(m2/2)) a=-a;
      
      kmin = abs(ja-jb);
      kmax = ja + jb;
      c = 1.0/((ja+1.0)*(jb+1.0));
      for (k = kmin; k <= kmax; k += 2) {
	if (k == m2) {	  
	  b = 1.0/(m2+1.0)-c;
	} else {
	  b = -c;
	}
	a += b*GKB(k0, k1, k);
      }
      if (IsEven((ja+jb)/2)) a = -a;
      e *= a;
    }
  }

  return e;
}

/*
** when enabled, qr_norm stores the shift of orbital energies for MBPT
*/
void ShiftOrbitalEnergy(CONFIG *cfg) {
  int i, j;
  ORBITAL *orb;
  CONFIG c;
  double e0, e1;

  c.n_shells = cfg->n_shells+1;
  c.shells = malloc(sizeof(SHELL)*c.n_shells);
  memcpy(c.shells+1, cfg->shells, sizeof(SHELL)*cfg->n_shells);
  c.shells[0].nq = 1;
  c.shells[1].nq -= 1;
  for (i = 0; i < n_orbitals; i++) {
    orb = GetOrbital(i);
    orb->qr_norm = 0.0;
    /* this disables the shift */
    continue;
    for (j = 0; j < cfg->n_shells; j++) {
      if (cfg->shells[j].n == orb->n && cfg->shells[j].kappa == orb->kappa) {
	break;
      }
    }
    ResidualPotential(&e0, i, i);
    if (j < cfg->n_shells) {
      e1 = CoulombEnergyShell(cfg, j);
    } else {
      c.shells[0].n = orb->n;
      c.shells[0].kappa = orb->kappa;
      e1 = CoulombEnergyShell(&c, 0);
    }
    orb->qr_norm = 2*e1 + e0;
  }
  free(c.shells);
}
    
double CoulombEnergyShell(CONFIG *cfg, int i) {
  int n, kappa, kl, j2, nq, k, np, kappap, klp, j2p;
  int nqp, kp, kk, j, kkmin, kkmax;
  double y, t, q, b, a, r;

  n = (cfg->shells[i]).n;
  kappa = (cfg->shells[i]).kappa;
  kl = GetLFromKappa(kappa);
  j2 = GetJFromKappa(kappa);
  nq = (cfg->shells[i]).nq;
  k = OrbitalIndex(n, kappa, 0.0);
    
  if (nq > 1) {
    t = 0.0;
    for (kk = 2; kk <= j2; kk += 2) {
      Slater(&y, k, k, k, k, kk, 0);
      q = W3j(j2, 2*kk, j2, -1, 0, 1);
      t += y * q * q ;
    }
    Slater(&y, k, k, k, k, 0, 0);
    b = (nq-1.0) * (y - (1.0 + 1.0/j2)*t);

  } else {
    b = 0.0;
  }

  t = 0.0;
  for (j = 0; j < cfg->n_shells; j++) {
    if (j == i) continue;
    nqp = (cfg->shells[j]).nq;
    if (nqp == 0) continue;
    np = (cfg->shells[j]).n;
    kappap = (cfg->shells[j]).kappa;
    klp = GetLFromKappa(kappap);
    j2p = GetJFromKappa(kappap);
    kp = OrbitalIndex(np, kappap, 0.0);
    
    kkmin = abs(j2 - j2p);
    kkmax = (j2 + j2p);
    if (IsOdd((kkmin + kl + klp)/2)) kkmin += 2;
    a = 0.0;
    for (kk = kkmin; kk <= kkmax; kk += 4) {
      Slater(&y, k, kp, kp, k, kk/2, 0);
      q = W3j(j2, kk, j2p, -1, 0, 1);
      a += y * q * q;
    }
    Slater(&y, k, kp, k, kp, 0, 0);
    t += nqp * (y - a);
  }
  
  r = 0.5*(b + t);
  
  return r;
}

double AverageEnergyConfig(CONFIG *cfg) {
  return AverageEnergyConfigMode(cfg, 0);
}

/* calculate the average energy of a configuration */
double AverageEnergyConfigMode(CONFIG *cfg, int md) {
  int i, j, n, kappa, nq, np, kappap, nqp, k2;
  int k, kp, kk, kl, klp, kkmin, kkmax, j2, j2p;
  double x, y, t, q, a, b, r, e, *v1, *v2;

  x = 0.0;
  for (i = 0; i < cfg->n_shells; i++) {
    n = (cfg->shells[i]).n;
    kappa = (cfg->shells[i]).kappa;
    kl = GetLFromKappa(kappa);
    j2 = GetJFromKappa(kappa);
    nq = (cfg->shells[i]).nq;
    k = OrbitalIndex(n, kappa, 0.0);
    if (md < 10) {
      double am = AMU * GetAtomicMass();
      if (nq > 1) {
	t = 0.0;
	for (kk = 1; kk <= j2; kk += 1) {
	  k2 = kk*2;
	  y = 0;
	  if (kk == 1 && qed.sms) {
	    v1 = Vinti(k, k);
	  } else {
	    v1 = NULL;
	  }
	  if (IsEven(kk)) {
	    Slater(&y, k, k, k, k, kk, 0);
	    if (k2 < MAXKSSC && k < MAXNSSC) {
	      y *= _slater_scale[k2][k][k];
	    }
	  }
	  if (v1 && qed.sms == 3) {
	    double a1 = ReducedCL(j2, 2*kk, j2);
	    if (fabs(a1) > 1e-10) {
	      y += 0.25*v1[2]*v1[2]/(am*a1*a1);
	    }	  
	    y += 0.25*v1[1]*v1[1]/am;
	  }
	  if (qed.br < 0 || n <= qed.br) {
	    if (qed.minbr <= 0 || n <= qed.minbr) {
	      int mbr = qed.mbr;
	      if (qed.nbr > 0 && n > qed.nbr) mbr = 0;
	      y += Breit(k, k, k, k, kk, kappa, kappa, kappa, kappa,
			 kl, kl, kl, kl, mbr);
	    }
	  }
	  if (y) {
	    q = W3j(j2, 2*kk, j2, -1, 0, 1);
	    t += y * q * q ;
	  }
	}
	Slater(&y, k, k, k, k, 0, 0);
	if (MAXKSSC > 0 && k < MAXNSSC) {
	  y *= _slater_scale[0][k][k];
	}
	b = ((nq-1.0)/2.0) * (y - (1.0 + 1.0/j2)*t);
      } else {
	b = 0.0;
      }
      
      t = 0.0;
      for (j = 0; j < i; j++) {
	np = (cfg->shells[j]).n;
	kappap = (cfg->shells[j]).kappa;
	klp = GetLFromKappa(kappap);
	j2p = GetJFromKappa(kappap);
	nqp = (cfg->shells[j]).nq;
	kp = OrbitalIndex(np, kappap, 0.0);
	kkmin = abs(j2 - j2p);
	kkmax = (j2 + j2p);
	int maxn, minn;
	if (n < np) {
	  minn = n;
	  maxn = np;
	} else {
	  minn = np;
	  maxn = n;
	}
	//if (IsOdd((kkmin + kl + klp)/2)) kkmin += 2;
	a = 0.0;
	for (kk = kkmin; kk <= kkmax; kk += 2) {
	  y = 0;
	  int kk2 = kk/2;
	  if (kk == 2 && qed.sms) {
	    v1 = Vinti(k, kp);
	    v2 = Vinti(kp, k);
	  } else {
	    v1 = NULL;
	    v2 = NULL;
	  }
	  if (IsEven((kl+klp+kk)/2)) {
	    Slater(&y, k, kp, kp, k, kk2, 0);
	    if (kk+1 < MAXKSSC && k < MAXNSSC && kp < MAXNSSC) {
	      y *= _slater_scale[kk+1][k][kp];
	    }
	    if (v1 && v2) {	    
	      y -= (v1[0]+v1[2])*v2[0]/am;
	    }
	  }
	  if (v1 && v2) {
	    double a1 = ReducedCL(j2, kk, j2p);
	    double a2 = ReducedCL(j2p, kk, j2);
	    if (IsEven((kl+klp+kk)/2)) {
	      y -= v1[1]*v2[0]/am;
	      if (qed.sms == 3 && fabs(a1) > 1e-10) {
		y += 0.5*(0.5+a2)*v1[2]*v2[2]/(am*a1*a2);
	      }
	    } else {
	      if (qed.sms == 3 && fabs(a1) > 1e-10) {
		y += 0.25*v1[2]*v2[2]/(am*a1*a2);
	      }
	    }
	    if (qed.sms == 3) {
	      y += 0.25*v1[1]*v2[1]/am;
	    }
	  }
	  if (qed.br < 0 || maxn <= qed.br) {
	    if (qed.minbr <= 0 || minn <= qed.minbr) {
	      int mbr = qed.mbr;
	      if (qed.nbr > 0 && maxn > qed.nbr) mbr = 0;
	      y += Breit(k, kp, kp, k, kk2, kappa, kappap, kappap, kappa,
			 kl, klp, klp, kl, mbr);
	    }
	  }
	  if (y) {
	    q = W3j(j2, kk, j2p, -1, 0, 1);
	    a += y * q * q;
	  }
	}
	y = 0;
	Slater(&y, k, kp, k, kp, 0, 0);
	if (MAXKSSC > 1 && k < MAXNSSC && kp < MAXNSSC) {
	  y *= _slater_scale[1][k][kp];
	}
	t += nqp * (y - a);
      }    
      ResidualPotential(&y, k, k);
      e = GetOrbital(k)->energy;
      a = QED1E(k, k);
      r = nq * (b + t + e + a + y);      
    } else {
      ORBITAL *orb = GetOrbitalSolved(k);
      a = SelfEnergy(orb, orb);
      r = nq * a;
    }
    x += r;
  }

  return x;
}

void DiExConfig(CONFIG *cfg, double *d0, double *d1) {
  int i, j, n, kappa, np, kappap;
  int k, kp, kk, kl, klp, kkmin, kkmax, j2, j2p;
  double y, t, q, a, b, nq, nqp;
  SHELL *s1, *s2;
 
  *d0 = 0.0;
  *d1 = 0.0;
  for (i = 0; i < cfg->n_shells; i++) {
    s1 = &cfg->shells[i];
    n = s1->n;
    kappa = s1->kappa;
    kl = GetLFromKappa(kappa);
    j2 = GetJFromKappa(kappa);
    nq = s1->nq;
    k = OrbitalIndex(n, kappa, 0.0);

    t = 0.0;
    for (kk = 2; kk <= j2; kk += 2) {
      Slater(&y, k, k, k, k, kk, 0);
      q = W3j(j2, 2*kk, j2, -1, 0, 1);
      t += y * q * q ;
    }
    Slater(&y, k, k, k, k, 0, 0);
    b = ((nq-1.0)/2.0);
    *d0 += nq*b*(y - (1.0+1.0/j2)*t);
    
#if FAC_DEBUG
      fprintf(debug_log, "\nAverage Radial: %lf\n", y);
#endif
      
    for (j = 0; j < i; j++) {
      s2 = &cfg->shells[j];
      np = s2->n;
      kappap = s2->kappa;
      klp = GetLFromKappa(kappap);
      j2p = GetJFromKappa(kappap);
      nqp = s2->nq;
      kp = OrbitalIndex(np, kappap, 0.0);

      kkmin = abs(j2 - j2p);
      kkmax = (j2 + j2p);
      if (IsOdd((kkmin + kl + klp)/2)) kkmin += 2;
      a = 0.0;
      for (kk = kkmin; kk <= kkmax; kk += 4) {
	Slater(&y, k, kp, kp, k, kk/2, 0);
	q = W3j(j2, kk, j2p, -1, 0, 1);
	a += y * q * q;
#if FAC_DEBUG
	fprintf(debug_log, "exchange rank: %d, q*q: %lf, Radial: %lf\n", 
		kk/2, q*q, y);
#endif

      }
      Slater(&y, k, kp, k, kp, 0, 0);

#if FAC_DEBUG
      fprintf(debug_log, "direct: %lf\n", y);
#endif
      *d0 += nq*nqp*y;
      *d1 += -nq*nqp*a;
    }
  }
}

/* calculate the average energy of an average configuration, 
** with seperate direct and exchange contributions */
void DiExAvgConfig(AVERAGE_CONFIG *cfg, double *d0, double *d1) {
  int i, j, n, kappa, np, kappap;
  int k, kp, kk, kl, klp, kkmin, kkmax, j2, j2p;
  double y, t, q, a, b, nq, nqp;
 
  *d0 = 0.0;
  *d1 = 0.0;
  for (i = 0; i < cfg->n_shells; i++) {
    n = cfg->n[i];
    kappa = cfg->kappa[i];
    kl = GetLFromKappa(kappa);
    j2 = GetJFromKappa(kappa);
    nq = cfg->nq[i];
    k = OrbitalIndex(n, kappa, 0.0);

    t = 0.0;
    for (kk = 2; kk <= j2; kk += 2) {
      Slater(&y, k, k, k, k, kk, 0);
      q = W3j(j2, 2*kk, j2, -1, 0, 1);
      t += y * q * q ;
    }
    Slater(&y, k, k, k, k, 0, 0);
    b = ((nq-1.0)/2.0);
    *d0 += nq*b*(y - (1.0+1.0/j2)*t);
    
#if FAC_DEBUG
      fprintf(debug_log, "\nAverage Radial: %lf\n", y);
#endif
      
    for (j = 0; j < i; j++) {
      np = cfg->n[j];
      kappap = cfg->kappa[j];
      klp = GetLFromKappa(kappap);
      j2p = GetJFromKappa(kappap);
      nqp = cfg->nq[j];
      kp = OrbitalIndex(np, kappap, 0.0);

      kkmin = abs(j2 - j2p);
      kkmax = (j2 + j2p);
      if (IsOdd((kkmin + kl + klp)/2)) kkmin += 2;
      a = 0.0;
      for (kk = kkmin; kk <= kkmax; kk += 4) {
	Slater(&y, k, kp, kp, k, kk/2, 0);
	q = W3j(j2, kk, j2p, -1, 0, 1);
	a += y * q * q;
#if FAC_DEBUG
	fprintf(debug_log, "exchange rank: %d, q*q: %lf, Radial: %lf\n", 
		kk/2, q*q, y);
#endif

      }
      Slater(&y, k, kp, k, kp, 0, 0);

#if FAC_DEBUG
      fprintf(debug_log, "direct: %lf\n", y);
#endif
      *d0 += nq*nqp*y;
      *d1 += -nq*nqp*a;
    }
  }
}

/* calculate the average energy of an average configuration */
double AverageEnergyAvgConfig(AVERAGE_CONFIG *cfg) {
  int i, j, n, kappa, np, kappap;
  int k, kp, kk, kl, klp, kkmin, kkmax, j2, j2p;
  double x, y, t, q, a, b, r, nq, nqp, r0, r1;
 
  r0 = 0.0;
  r1 = 0.0;
  x = 0.0;
  for (i = 0; i < cfg->n_shells; i++) {
    n = cfg->n[i];
    kappa = cfg->kappa[i];
    kl = GetLFromKappa(kappa);
    j2 = GetJFromKappa(kappa);
    nq = cfg->nq[i];
    k = OrbitalIndex(n, kappa, 0.0);
    
    t = 0.0;
    for (kk = 2; kk <= j2; kk += 2) {
      Slater(&y, k, k, k, k, kk, 0);
      q = W3j(j2, 2*kk, j2, -1, 0, 1);
      t += y * q * q ;
    }
    Slater(&y, k, k, k, k, 0, 0);
    b = ((nq-1.0)/2.0) * (y - (1.0 + 1.0/j2)*t);
    
#if FAC_DEBUG
      fprintf(debug_log, "\nAverage Radial: %lf\n", y);
#endif
      
    t = 0.0;
    for (j = 0; j < i; j++) {
      np = cfg->n[j];
      kappap = cfg->kappa[j];
      klp = GetLFromKappa(kappap);
      j2p = GetJFromKappa(kappap);
      nqp = cfg->nq[j];
      kp = OrbitalIndex(np, kappap, 0.0);

      kkmin = abs(j2 - j2p);
      kkmax = (j2 + j2p);
      if (IsOdd((kkmin + kl + klp)/2)) kkmin += 2;
      a = 0.0;
      for (kk = kkmin; kk <= kkmax; kk += 4) {
	Slater(&y, k, kp, kp, k, kk/2, 0);
	q = W3j(j2, kk, j2p, -1, 0, 1);
	a += y * q * q;
#if FAC_DEBUG
	fprintf(debug_log, "exchange rank: %d, q*q: %lf, Radial: %lf\n", 
		kk/2, q*q, y);
#endif

      }
      Slater(&y, k, kp, k, kp, 0, 0);

#if FAC_DEBUG
      fprintf(debug_log, "direct: %lf\n", y);
#endif

      t += nqp * (y - a);
    }

    ResidualPotential(&y, k, k);
    a = GetOrbital(k)->energy;
    r = nq * (b + t + a + y);
    r0 += nq*y;
    r1 += nq*(b+t);
    x += r;
  }

  /*printf("%12.5E %12.5E %15.8E\n", r0, r1, x);*/
  return x;
}

double ZerothHamilton(ORBITAL *orb0, ORBITAL *orb1) {
  double *p0, *p1, *q0, *q1;
  if (orb0->kappa != orb1->kappa) return 0.0;
  
  p0 = Large(orb0);
  p1 = Large(orb1);
  q0 = Small(orb0);
  q1 = Small(orb1);
  int ilast = Min(orb0->ilast, orb1->ilast);
  int i;
  double v, a, a2, b;
  Differential(p1, _xk, 0, ilast, potential);
  Differential(q1, _zk, 0, ilast, potential);
  a = 1/FINE_STRUCTURE_CONST;
  a2 = 1/FINE_STRUCTURE_CONST2;
  for (i = 0; i <= ilast; i++) {
    v = potential->Vc[i] + potential->U[i];
    b = orb1->kappa/potential->rad[i];
    _yk[i] = p0[i]*p1[i]*(v) + a*p0[i]*(-_zk[i]+q1[i]*b);
    _yk[i] += q0[i]*q1[i]*(v-2*a2) + a*q0[i]*(_xk[i]+p1[i]*b);
    _yk[i] *= potential->dr_drho[i];
    
  }
  _xk[0] = 0;
  NewtonCotes(_xk, _yk, 0, ilast, 1, 0);
  b = _xk[ilast];
  //b = Simpson(_yk, 0, ilast);
  return b;
}

double ConfigHamilton(CONFIG *cfg, ORBITAL *orb0, ORBITAL *orb1, double xdf) {
  double t, y, a, nqp, q, kappa;
  int k0, k1, j2, kl, j2p, klp, minn, maxn;
  int np, kp, kappap, kk, kkmin, kkmax, kk2, j;

  if (orb0->kappa != orb1->kappa) return 0;
  k0 = orb0->idx;
  k1 = orb1->idx;
  kappa = orb0->kappa;  
  GetJLFromKappa(kappa, &j2, &kl);
  if (orb1->n > orb0->n) {
    maxn = orb1->n;
    minn = orb0->n;
  } else {
    maxn = orb0->n;
    minn = orb1->n;
  }
  t = 0.0;
  double te = 0.0;
  double td = 0.0;
  for (j = 0; j < cfg->n_shells; j++) {
    np = (cfg->shells[j]).n;
    kappap = (cfg->shells[j]).kappa;
    kp = OrbitalIndex(np, kappap, 0);
    GetJLFromKappa(kappap, &j2p, &klp);
    nqp = (cfg->shells[j]).nq;
    if (kappap == kappa) {
      if (np == orb0->n) nqp -= xdf;
      if (np == orb1->n) nqp -= xdf;
    }
    if (nqp <= 0) continue;
    kkmin = abs(j2 - j2p);
    kkmax = (j2 + j2p);
    if (np > maxn) maxn = np;
    if (np < minn) minn = np;
    a = 0.0;
    for (kk = kkmin; kk <= kkmax; kk += 2) {
      y = 0;
      int kk2 = kk/2;
      if (IsEven((kl + klp + kk)/2)) {
	Slater(&y, k0, kp, kp, k1, kk2, 0);
      }
      /*
      if (qed.br < 0 || maxn <= qed.br) {
	if (qed.minbr <= 0 || minn <= qed.minbr) {
	  int mbr = qed.mbr;
	  if (qed.nbr > 0 && maxn > qed.nbr) mbr = 0;
	  y += Breit(k0, kp, kp, k0, kk, kappa, kp, kp, kappa,
		     kl, klp, klp, kl, mbr);
	}
      }
      */
      if (y) {
	q = W3j(j2, kk, j2p, -1, 0, 1);
	a += y*q*q;
      }
    }
    te -= nqp*a;
    y = 0;
    Slater(&y, k0, kp, k1, kp, 0, 0);
    td += nqp*y;
  }
  y = 0;
  ResidualPotential(&y, k0, k1);
  t = td + te + y;

  return t;
}

/* calculate the expectation value of the residual potential:
   -Z/r - v0(r), where v0(r) is central potential used to solve 
   dirac equations. the orbital index must be valid, i.e., their 
   radial equations must have been solved. */
int ResidualPotential(double *s, int k0, int k1) {
  int i;
  ORBITAL *orb1, *orb2;
  int index[2];
  LOCK *lock = NULL;
  double *p, z, *p1, *p2, *q1, *q2;

  orb1 = GetOrbitalSolved(k0);
  orb2 = GetOrbitalSolved(k1);
  if (!orb1 || !orb2) return -1;
  if (orb1->wfun == NULL || orb2->wfun == NULL) {
    *s = 0.0;
    return 0;
  }

  if (k0 > k1) {
    index[0] = k1;
    index[1] = k0;
  } else {
    index[0] = k0;
    index[1] = k1;
  }
  int myrank = MyRankMPI()+1;
  p = (double *) MultiSet(residual_array, index, NULL, &lock,
			  InitDoubleData, NULL);
  double pp;
#pragma omp atomic read
  pp = *p;
  int locked = 0;
  if (lock && !(p && pp)) {
    SetLock(lock);
    locked = 1;
#pragma omp atomic read
    pp = *p;
  }
  if (p && pp) {
    *s = pp;
#pragma omp flush
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    residual_array->iset -= myrank;
#pragma omp flush
    return 0;
  } 

  *s = 0.0;
  if (orb1->n < 0 || orb2->n < 0) {
    p1 = Large(orb1);
    p2 = Large(orb2);
    q1 = Small(orb1);
    q2 = Small(orb2);
    for (i = potential->ib; i <= potential->ib1; i++) {
      z = potential->Vc[i] + potential->U[i];
      _yk[i] = -(potential->Z[i]/potential->rad[i]) - z;
      _yk[i] *= potential->dr_drho[i];
      _yk[i] *= p1[i]*p2[i] + q1[i]*q2[i];
    }
    *s = Simpson(_yk, potential->ib, potential->ib1);
  } else {
    for (i = 0; i < potential->maxrp; i++) {
      z = potential->Vc[i] + potential->U[i];
      _yk[i] = -(potential->Z[i]/potential->rad[i]) - z;
    }
    Integrate(_yk, orb1, orb2, 1, s, -1);
    if (potential->nfrozen || potential->npseudo || potential->sturm_idx > 0) {
      z = ZerothHamilton(orb1, orb2);
      *s += z;
      if (orb1->n == orb2->n) *s -= orb1->energy;
    }
  }
#pragma omp atomic write
  *p = *s;
#pragma omp flush

  if (locked) ReleaseLock(lock);
#pragma omp atomic
  residual_array->iset -= myrank;
#pragma omp flush
  return 0;
}

double MeanPotential(int k0, int k1) {
  int i;
  ORBITAL *orb1, *orb2;
  double z, *p1, *p2, *q1, *q2;

  orb1 = GetOrbitalSolved(k0);
  orb2 = GetOrbitalSolved(k1);
  if (!orb1 || !orb2) return -1;
  if (orb1->wfun == NULL || orb2->wfun == NULL) {
    return 0.0;
  }

  if (orb1->n < 0 || orb2->n < 0) {
    p1 = Large(orb1);
    p2 = Large(orb2);
    q1 = Small(orb1);
    q2 = Small(orb2);
    for (i = potential->ib; i <= potential->ib1; i++) {
      z = potential->U[i];
      z += potential->Vc[i];
      _yk[i] = z;
      _yk[i] *= potential->dr_drho[i];
      _yk[i] *= p1[i]*p2[i] + q1[i]*q2[i];
    }
    z = Simpson(_yk, potential->ib, potential->ib1);
  } else {
    for (i = 0; i < potential->maxrp; i++) {
      z = potential->U[i];
      z += potential->Vc[i];
      _yk[i] = z;
    }
    Integrate(_yk, orb1, orb2, 1, &z, -1);
  }

  return z;
}

double RadialMoments(int m, int k1, int k2) {
  int index[3];
  LOCK *lock = NULL;
  int npts, i0, i;
  ORBITAL *orb1, *orb2;
  double *q, r, z, *p1, *p2, *q1, *q2;
  int n1, n2;
  int kl1, kl2;
  int nh, klh;
  
  orb1 = GetOrbitalSolved(k1);
  orb2 = GetOrbitalSolved(k2);
  n1 = orb1->n;
  n2 = orb2->n;
  kl1 = orb1->kappa;
  kl2 = orb2->kappa;
  kl1 = GetLFromKappa(kl1);
  kl2 = GetLFromKappa(kl2);
  kl1 /= 2;
  kl2 /= 2;	

  GetHydrogenicNL(&nh, &klh, NULL, NULL);

  if (n1 > 0 && n2 > 0 && potential->ib <= 0) {
    if ((n1 > nh && n2 > nh) || 
	(kl1 > klh && kl2 > klh) ||
	orb1->wfun == NULL || 
	orb2->wfun == NULL) {
      if (n1 == n2 && kl1 == kl2) {
	z = GetResidualZ();
	r = HydrogenicExpectation(z, m, n1, kl1);
	if (r) {
	  return r;
	}
      } else if (m == 1) {
	z = GetResidualZ();
	if (n1 < n2) {
	  r = HydrogenicDipole(z, n1, kl1, n2, kl2);
	  return r;
	} else if (n1 < n2) {
	  r = HydrogenicDipole(z, n2, kl2, n1, kl1);
	  return r;
	}
      }
    }
  }

  if (potential->ib <= 0 && n1 == n2 && m > 0 && n1 > potential->nmax) {
    return 0.0;
  }
  if (orb1->wfun == NULL || orb2->wfun == NULL) {
    return 0.0;
  }

  if (m >= 0) {
    index[0] = 2*m;
  } else {
    index[0] = -2*m-1;
  }
    
  if (k1 < k2) {
    index[1] = k1;
    index[2] = k2;
  } else {
    index[1] = k2;
    index[2] = k1;
  }
  int myrank = MyRankMPI()+1;
  q = (double *) MultiSet(moments_array, index, NULL, &lock,
			  InitDoubleData, NULL);
  double qd;
#pragma omp atomic read
  qd = *q;
  int locked = 0;
  if (lock && !qd) {
    SetLock(lock);
    locked = 1;
#pragma omp atomic read
    qd = *q;
  }
  if (qd) {
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    moments_array->iset -= myrank;
#pragma omp flush
    return *q;
  } 
  if (n1 < 0 || n2 < 0) {
    i0 = potential->ib;
    npts = potential->ib1;
    p1 = Large(orb1);
    q1 = Small(orb1);
    p2 = Large(orb2);
    q2 = Small(orb2);
    for (i = i0; i <= npts; i++) {
      r = p1[i]*p2[i] + q1[i]*q2[i];
      r *= potential->dr_drho[i];
      _yk[i] = r;
      if (m != 0) _yk[i] *= pow(potential->rad[i], m);
    }
    r = Simpson(_yk, i0, npts);
#pragma omp atomic write
    *q = r;
  } else {    
    npts = potential->maxrp-1;
    if (n1 != 0) npts = Min(npts, orb1->ilast);
    if (n2 != 0) npts = Min(npts, orb2->ilast);
    if (m == 0) {
      for (i = 0; i <= npts; i++) {
	_yk[i] = 1.0;
      }
    } else {
      for (i = 0; i <= npts; i++) {
	_yk[i] = pow(potential->rad[i], m);
      }
    }
    r = 0.0;
    Integrate(_yk, orb1, orb2, 1, &r, m);
#pragma omp atomic write
    *q = r;
#pragma omp flush
  }
  if (locked) ReleaseLock(lock);
#pragma omp atomic
    moments_array->iset -= myrank;
#pragma omp flush
  return r;
}

double MultipoleRadialNR(int m, int k1, int k2, int gauge) {
  int i, p, t;
  ORBITAL *orb1, *orb2;
  double r;
  int kappa1, kappa2;

#ifdef PERFORM_STATISTICS
  clock_t start, stop; 
  start = clock();
#endif

  orb1 = GetOrbital(k1);
  orb2 = GetOrbital(k2);
  kappa1 = orb1->kappa;
  kappa2 = orb2->kappa;

  r = 0.0;
  if (m == 1) {
    /* use the relativistic version is just simpler */
    printf("should call MultipoleRadialFR instead\n");
  } else if (m > 1) {
    t = kappa1 + kappa2;
    p = m - t;
    if (p && t) {
      r = RadialMoments(m-1, k1, k2);
      r *= p*t;
      r /= sqrt(m*(m+1.0));
      r *= -0.5 * FINE_STRUCTURE_CONST;
      for (i = 2*m - 1; i > 0; i -= 2) r /= (double) i;
      r *= ReducedCL(GetJFromKappa(kappa1), 2*m, GetJFromKappa(kappa2));
    }
  } else if (m < 0) {
    m = -m;
    if (gauge == G_BABUSHKIN) {
      r = RadialMoments(m, k1, k2);
      r *= sqrt((m+1.0)/m);
      for (i = 2*m - 1; i > 1; i -= 2) r /= (double) i;
    } else {
      /* the velocity form is not implemented yet. 
	 the following is still the length form */
      r = RadialMoments(m, k1, k2);
      r *= sqrt((m+1.0)/m);
      for (i = 2*m - 1; i > 1; i -= 2) r /= (double) i;
    }
    r *= ReducedCL(GetJFromKappa(kappa1), 2*m, GetJFromKappa(kappa2));
  }

#ifdef PERFORM_STATISTICS 
  stop = clock();
  rad_timing.radial_1e += stop - start;
#endif
  
  return r;
}

/* 
** for |m|>=MMFR, the relativistic multipole moments are 
** multiplied by a^|m| to avoid under/overflow
*/
int MultipoleRadialFRGrid(double **p0, int m, int k1, int k2, int gauge) {
  double q, ip, ipm, im, imm;
  int kappa1, kappa2;
  int am, t;
  int index[4], s;
  ORBITAL *orb1, *orb2, *orb;
  double x, a, r, rp, ef, **p1;
  int jy, n, i, j, npts;
  double rcl;

#ifdef PERFORM_STATISTICS 
  clock_t start, stop;
  start = clock();
#endif

  if (m == 0) return 0;

  if (_nmp > 0) {
    int nmp2 = _nmp*_nmp;
    t = m < 0?0:1;
    j = gauge==G_COULOMB?0:1;
    i = abs(m)-1;
    double *y = _dmp[t][j][i];
    if (y != NULL) {
      orb1 = GetOrbital(k1);
      orb2 = GetOrbital(k2);
      if (orb1->n > 0 && orb1->n <= _nmp &&
	  orb2->n > 0 && orb2->n <= _nmp) {
	ip = ShellToInt(orb1->n, orb1->kappa);
	im = ShellToInt(orb2->n, orb2->kappa);
	n = (ip*nmp2+im)*n_awgrid;
	*p0 = y+n;
	return n_awgrid;
      }
    }
  }
  
  if (m >= 0) {
    index[0] = 2*m;
    am = m;
  } else {
    index[0] = -2*m-1;
    am = -m;
  }
  index[1] = k1;
  index[2] = k2;
  LOCK *lock = NULL;
  int myrank = MyRankMPI()+1;
  p1 = (double **) MultiSet(multipole_array, index, NULL, &lock,
			    InitPointerData, FreeMultipole);
  double *pp;
#pragma omp atomic read
  pp = *p1;
  int locked = 0;
  if (lock && !pp) {
    SetLock(lock);
    locked = 1;
#pragma omp atomic read
    pp = *p1;
  }
  if (pp) {
    *p0 = pp;
#pragma omp flush
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    multipole_array->iset -= myrank;
#pragma omp flush
    return n_awgrid;
  }

  orb1 = GetOrbitalSolved(k1);
  orb2 = GetOrbitalSolved(k2);
  
  kappa1 = orb1->kappa;
  kappa2 = orb2->kappa;
  rcl = ReducedCL(GetJFromKappa(kappa1), abs(2*m), 
		  GetJFromKappa(kappa2));
  double *pt = (double *) malloc(sizeof(double)*n_awgrid);
  if (fabs(rcl) < EPS10) {
    for (i = 0; i < n_awgrid; i++) {
      pt[i] = 0;
    }
  } else {
    ef = 0;
    if (orb1->n == 0 || orb2->n == 0) {
      ef = Max(orb1->energy, orb2->energy);
      if (ef > 0.0) {
	ef *= FINE_STRUCTURE_CONST;
      } else {
	ef = 0.0;
      }
    }
    
    npts = potential->maxrp-1;
    if (orb1->n > 0) npts = Min(npts, orb1->ilast);
    if (orb2->n > 0) npts = Min(npts, orb2->ilast);
    r = 0.0;
    jy = 1;
    
    for (i = 0; i < n_awgrid; i++) {
      r = 0.0;
      pt[i] = 0.0;
      if (uta_awgrid) {
	a = fabs(orb1->energy-orb2->energy)*FINE_STRUCTURE_CONST;
      } else {
	a = awgrid[i];
	if (ef > 0.0) a += ef;
      }
      if (m > 0) {
	t = kappa1 + kappa2;
	if (t) {
	  for (j = 0; j <= npts; j++) {
	    x = a*potential->rad[j];
	    n = m;
	    _yk[j] = BESLJN(jy, n, x);
	  }
	  Integrate(_yk, orb1, orb2, 4, &r, 0);
	  r *= t;
	  r *= (2*m + 1.0)/sqrt(m*(m+1.0));
	  if (m < MMFR) r /= pow(a, m);
	  pt[i] = r*rcl;
	}
      } else {
	if (gauge == G_COULOMB) {
	  t = kappa1 - kappa2;
	  q = sqrt(am/(am+1.0));
	  for (j = 0; j <= npts; j++) {
	    x = a*potential->rad[j];
	    n = am+1;
	    _yk[j] = BESLJN(jy, n, x);
	    n = am-1;
	    _zk[j] = BESLJN(jy, n, x);
	  }
	  r = 0.0;
	  rp = 0.0;
	  if (t) {
	    Integrate(_yk, orb1, orb2, 4, &ip, 0);
	    Integrate(_zk, orb1, orb2, 4, &ipm, 0);
	    r = t*ip*q - t*ipm/q;
	  }
	  if (k1 != k2) {
	    Integrate(_yk, orb1, orb2, 5, &im, 0);
	    Integrate(_zk, orb1, orb2, 5, &imm, 0);
	    rp = (am + 1.0)*im*q + am*imm/q;
	  }
	  r += rp;
	  if (am > 1) {
	    if (am < MMFR) {
	      r /= pow(a, am-1);
	    } else {
	      r *= a;
	    }
	  }
	  pt[i] = r*rcl;
	} else if (gauge == G_BABUSHKIN) {
	  t = kappa1 - kappa2;
	  for (j = 0; j <= npts; j++) {
	    x = a*potential->rad[j];
	    n = am+1;
	    _yk[j] = BESLJN(jy, n, x);
	    n = am;
	    _zk[j] = BESLJN(jy, n, x);
	  }
	  if (t) {
	    Integrate(_yk, orb1, orb2, 4, &ip, 0);
	    r = t*ip;
	  }
	  if (k1 != k2) {
	    Integrate(_yk, orb1, orb2, 5, &im, 0);
	  } else {
	    im = 0.0;
	  }
	  Integrate(_zk, orb1, orb2, 1, &imm, 0);
	  rp = (am + 1.0) * (imm + im);
	  q = (2*am + 1.0)/sqrt(am*(am+1.0));
	  if (am < MMFR) q /= pow(a, am);
	  r *= q;
	  rp *= q;
	  pt[i] = (r+rp)*rcl;
	}
      }
    }
  }
#ifdef PERFORM_STATISTICS 
  stop = clock();
  rad_timing.radial_1e += stop - start;
#endif
#pragma omp atomic write
  *p0 = pt;
#pragma omp atomic write
  *p1 = pt;
#pragma omp flush
  if (locked) ReleaseLock(lock);
#pragma omp atomic
    multipole_array->iset -= myrank;
#pragma omp flush
  return n_awgrid;
}

double IntRadJn(int n0, int k0, double e0,
		int n1, int k1, double e1,
		int n, int m, char *fn) {
  ORBITAL *orb0, *orb1;
  int i, i0, i1, jy;
  double de, a, x, r;

  i0 = OrbitalIndex(n0, k0, e0);
  i1 = OrbitalIndex(n1, k1, e1);
  orb0 = GetOrbital(i0);
  orb1 = GetOrbital(i1);
  jy = 1;
  de = fabs(orb1->energy - orb0->energy);
  a = FINE_STRUCTURE_CONST*de;
  for (i = 0; i < potential->maxrp; i++) {
    if (n < 0) {
      _xk[i] = pow(potential->rad[i], n);
    } else {
      x = a*potential->rad[i];
      _xk[i] = BESLJN(jy, n, x);
    }
  }
  _yk[0] = 0.0;
  m = abs(m);

  Integrate(_xk, orb0, orb1, m, &r, 0);
  Integrate(_xk, orb0, orb1, -m, _yk, 0);
  if (fn) {
    FILE *f;
    f = fopen(fn, "w");
    if (f != NULL) {
      double *p0, *q0, *p1, *q1;
      p0 = Large(orb0);
      q0 = Small(orb0);
      p1 = Large(orb1);
      q1 = Small(orb1);
      fprintf(f, "# n0 = %d\n", n0);
      fprintf(f, "# k0 = %d\n", k0);
      fprintf(f, "# e0 = %12.5E\n", orb0->energy);
      fprintf(f, "# i0 = %d\n", orb0->ilast);
      fprintf(f, "# n1 = %d\n", n1);
      fprintf(f, "# k1 = %d\n", k1);
      fprintf(f, "# e1 = %12.5E\n", orb1->energy);
      fprintf(f, "# i1 = %d\n", orb1->ilast);
      for (i = 0; i < potential->maxrp; i++) {
	fprintf(f, "%4d %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E %15.8E\n",
		i, potential->rad[i], _xk[i], _yk[i],
		p0[i], q0[i], p1[i], q1[i]);
      }
      fclose(f);
    }
  }
  return r;
}

double MultipoleRadialFR(double aw, int m, int k1, int k2, int gauge) {
  int n;
  ORBITAL *orb1, *orb2;
  double *y, ef, r, aw0;

  orb1 = GetOrbitalSolved(k1);
  orb2 = GetOrbitalSolved(k2);
  if (orb1->wfun == NULL || orb2->wfun == NULL) {
    if (m == -1) {
      return MultipoleRadialNR(m, k1, k2, gauge);
    } else {
      return 0.0;
    }
  }
  
  n = MultipoleRadialFRGrid(&y, m, k1, k2, gauge);
  if (n == 0) return 0.0;
  ef = 0;
  if (orb1->n == 0 || orb2->n == 0) {
    ef = Max(orb1->energy, orb2->energy);
    if (ef > 0.0) {
      ef *= FINE_STRUCTURE_CONST;
    } else {
      ef = 0.0;
    }
  }
  aw0 = aw;
  if (ef > 0) aw0 += ef;

  r = InterpolateMultipole(aw, n, awgrid, y);
  if (gauge == G_COULOMB && m < 0) r /= aw0;

  return r;
}

/* fully relativistic multipole operator, 
   see Grant, J. Phys. B. 1974. Vol. 7, 1458. */ 
double MultipoleRadialFR0(double aw, int m, int k1, int k2, int gauge) {
  double q, ip, ipm, im, imm;
  int kappa1, kappa2;
  int am, t;
  int index[4];
  ORBITAL *orb1, *orb2, *orb;
  double x, a, r, rp, **p1, ef;
  int jy, n, i, j, npts;
  double rcl;

#ifdef PERFORM_STATISTICS 
  clock_t start, stop;
  start = clock();
#endif

  if (m == 0) return 0.0;
  
  if (m >= 0) {
    index[0] = 2*m;
    am = m;
  } else {
    index[0] = -2*m-1;
    am = -m;
  }

  orb1 = GetOrbitalSolved(k1);
  orb2 = GetOrbitalSolved(k2);
  if (orb1->wfun == NULL || orb2->wfun == NULL) {
    if (m == -1) {
      return MultipoleRadialNR(m, k1, k2, gauge);
    } else {
      return 0.0;
    }
  }
  
  index[1] = k1;
  index[2] = k2;
  kappa1 = orb1->kappa;
  kappa2 = orb2->kappa;
  rcl = ReducedCL(GetJFromKappa(kappa1), abs(2*m), 
		  GetJFromKappa(kappa2));
  ef = 0;
  if (orb1->n == 0 || orb2->n == 0) {
    ef = Max(orb1->energy, orb2->energy);
    if (ef > 0.0) {
      ef *= FINE_STRUCTURE_CONST;
    } else {
      ef = 0.0;
    }
  }
  if (n_awgrid > 1) {
    if (ef > 0) aw += ef;
  }
  LOCK *lock = NULL;
  int myrank = MyRankMPI()+1;
  p1 = (double **) MultiSet(multipole_array, index, NULL, &lock,
			    InitPointerData, FreeMultipole);
  double *pp;
#pragma omp atomic read
  pp = *p1;
  int locked = 0;
  if (lock && !pp) {
    SetLock(lock);
    locked = 1;
#pragma omp atomic read
    pp = *p1;
  }
  if (pp) {
    r = InterpolateMultipole(aw, n_awgrid, awgrid, pp);
    if (gauge == G_COULOMB && m < 0) r /= aw;
    r *= rcl;
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    multipole_array->iset -= myrank;
#pragma omp flush
    return r;
  }
  double *pt = (double *) malloc(sizeof(double)*n_awgrid);  
  npts = potential->maxrp-1;
  if (orb1->n > 0) npts = Min(npts, orb1->ilast);
  if (orb2->n > 0) npts = Min(npts, orb2->ilast);
  r = 0.0;
  jy = 1;

  for (i = 0; i < n_awgrid; i++) {
    r = 0.0;
    pt[i] = 0.0;
    if (uta_awgrid) {
      a = fabs(orb1->energy-orb2->energy)*FINE_STRUCTURE_CONST;
    } else {
      a = awgrid[i];
      if (ef > 0.0) a += ef;
    }
    if (m > 0) {
      t = kappa1 + kappa2;
      if (t) {
	for (j = 0; j <= npts; j++) {
	  x = a*potential->rad[j];
	  n = m;
	  _yk[j] = BESLJN(jy, n, x);
	}
	Integrate(_yk, orb1, orb2, 4, &r, 0);
	r *= t;
	r *= (2*m + 1.0)/sqrt(m*(m+1.0));
	r /= pow(a, m);
	pt[i] = r;
      }
    } else {
      if (gauge == G_COULOMB) {
	t = kappa1 - kappa2;
	q = sqrt(am/(am+1.0));
	for (j = 0; j <= npts; j++) {
	  x = a*potential->rad[j];
	  n = am+1;
	  _yk[j] = BESLJN(jy, n, x);
	  n = am-1;
	  _zk[j] = BESLJN(jy, n, x);
	}
	r = 0.0;
	rp = 0.0;
	if (t) {
	  Integrate(_yk, orb1, orb2, 4, &ip, 0);
	  Integrate(_zk, orb1, orb2, 4, &ipm, 0);
	  r = t*ip*q - t*ipm/q;
	}
	if (k1 != k2) {
	  Integrate(_yk, orb1, orb2, 5, &im, 0);
	  Integrate(_zk, orb1, orb2, 5, &imm, 0);
	  rp = (am + 1.0)*im*q + am*imm/q;
	}
	r += rp;
	if (am > 1) r /= pow(a, am-1);
	pt[i] = r;
      } else if (gauge == G_BABUSHKIN) {
	t = kappa1 - kappa2;
	for (j = 0; j < npts; j++) {
	  x = a*potential->rad[j];
	  n = am+1;
	  _yk[j] = BESLJN(jy, n, x);
	  n = am;
	  _zk[j] = BESLJN(jy, n, x);
	}
	if (t) {
	  Integrate(_yk, orb1, orb2, 4, &ip, 0);
	  r = t*ip;
	}
	if (k1 != k2) {
	  Integrate(_yk, orb1, orb2, 5, &im, 0);
	} else {
	  im = 0.0;
	}
	Integrate(_zk, orb1, orb2, 1, &imm, 0);
	rp = (am + 1.0) * (imm + im);
	q = (2*am + 1.0)/sqrt(am*(am+1.0));
	q /= pow(a, am);
	r *= q;
	rp *= q;
	pt[i] = r+rp;
      }
    }
  }

  r = InterpolateMultipole(aw, n_awgrid, awgrid, pt);
  if (gauge == G_COULOMB && m < 0) r /= aw;
  r *= rcl;

#ifdef PERFORM_STATISTICS 
  stop = clock();
  rad_timing.radial_1e += stop - start;
#endif
#pragma omp atomic write
  *p1 = pt;
#pragma omp flush
  if (locked) ReleaseLock(lock);
#pragma omp atomic
    multipole_array->iset -= myrank;
#pragma omp flush
  return r;
}

double *GeneralizedMoments(int k1, int k2, int m) {
  ORBITAL *orb1, *orb2;
  int n1, i, jy, nk;
  double x, r, r0;
  double *p1, *p2, *q1, *q2;
  int index[3], t;
  double **p, k, *kg;
  double amin, amax, kmin, kmax;
  
  index[0] = m;
  if (k1 > k2) {
    index[1] = k2;
    index[2] = k1;
    orb1 = GetOrbitalSolved(k2);
    orb2 = GetOrbitalSolved(k1);
  } else {
    index[1] = k1;
    index[2] = k2;
    orb1 = GetOrbitalSolved(k1);
    orb2 = GetOrbitalSolved(k2);
  }
  LOCK *lock = NULL;
  int myrank = MyRankMPI()+1;
  p = (double **) MultiSet(gos_array, index, NULL, &lock,
			   InitPointerData, FreeMultipole);
  double *pp;
#pragma omp atomic read
  pp = *p;
  int locked = 0;
  if (lock && !pp) {
    SetLock(lock);
    locked = 1;
#pragma omp atomic read
    pp = *p;
  }
  if (pp) {
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    gos_array->iset -= myrank;
#pragma omp flush
    return pp;
  }

  nk = NGOSK;
  double *pt = (double *) malloc(sizeof(double)*nk*2);
  kg = pt + nk;

  if (orb1->wfun == NULL || orb2->wfun == NULL || 
      (orb1->n <= 0 && orb2->n <= 0)) {
    for (t = 0; t < nk*2; t++) {
      pt[t] = 0.0;
    }
#pragma omp atomic write
    *p = pt;
#pragma omp flush
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    gos_array->iset -= myrank;
#pragma omp flush
    return *p;
  }
  
  jy = 1;
  p1 = Large(orb1);
  p2 = Large(orb2);
  q1 = Small(orb1);
  q2 = Small(orb2);

  amin = sqrt(2.0*fabs(orb1->energy));
  amax = sqrt(2.0*fabs(orb2->energy));
  if (amin < amax) {
    kmin = amin;
    kmax = amax;
  } else {
    kmin = amax;
    kmax = amin;
  }
  kmin = log(0.05*kmin);
  kmax = log(50.0*kmax);
  r = (kmax - kmin)/(nk-1.0);
  kg[0] = kmin;
  for (i = 1; i < nk; i++) {
    kg[i] = kg[i-1] + r;
  }
  
  if (orb1->n > 0 && orb2->n > 0) {
    n1 = Min(orb1->ilast, orb2->ilast);
  
    for (i = 0; i <= n1; i++) {
      _phase[i] = (p1[i]*p2[i] + q1[i]*q2[i])*potential->dr_drho[i];
    }
    
    if (m == 0) {
      if (k1 == k2) r0 = 1.0;
      else if (orb1->n != orb2->n) r0 = 0.0;
      else {
	if (orb1->kappa + orb2->kappa != -1) r0 = 0.0;
	else {
	  r0 = Simpson(_phase, 0, n1);
	}
      }
    } else {
      r0 = 0.0;
    }
    
    for (t = 0; t < nk; t++) {
      k = exp(kg[t]);
      for (i = 0; i <= n1; i++) {
	x = k * potential->rad[i];
	_dphase[i] = BESLJN(jy, m, x);
	_dphase[i] *= _phase[i];
      }
      r = Simpson(_dphase, 0, n1);
      
      pt[t] = (r - r0)/k;
    }
  } else {
    if (orb1->n > 0) n1 = orb1->ilast;
    else n1 = orb2->ilast;
    for (t = 0; t < nk; t++) {
      k = exp(kg[t]);      
      for (i = 0; i <= n1; i++) {
	x = k * potential->rad[i];
	_yk[i] = BESLJN(jy, m, x);
      }
      Integrate(_yk, orb1, orb2, 1, &r, 0);
      pt[t] = r/k;
    }
  }
  *p = pt;
#pragma omp flush
  if (locked) ReleaseLock(lock);
#pragma omp atomic
  gos_array->iset -= myrank;
#pragma omp flush
  return *p;
}

void PrintGeneralizedMoments(char *fn, int m, int n0, int k0, 
			     int n1, int k1, double e1) {
  FILE *f;
  int i0, i1, i;
  double *g, *x;

  i0 = OrbitalIndex(n0, k0, 0.0);
  e1 /= HARTREE_EV;
  i1 = OrbitalIndex(n1, k1, e1);
  f = fopen(fn, "w");
  if (f == NULL) {
    printf("cannot open file %s\n", fn);
    return;
  }
  g = GeneralizedMoments(i0, i1, m);
  x = g + NGOSK;
  for (i = 0; i < NGOSK; i++) {
    fprintf(f, "%15.8E %15.8E %15.8E\n", x[i], exp(x[i]), g[i]);
  }
  fclose(f);
}
  
double InterpolateMultipole(double aw, int n, double *x, double *y) {
  double r;
  int np, nd;

  if (n == 1) {
    r = y[0];
  } else {
    np = 3;
    nd = 1;
    if (aw <= x[0]) {
      r = y[0];
    } else if (aw >= x[n-1]) {
      r = y[n-1];
    } else {
      UVIP3P(np, n, x, y, nd, &aw, &r);
    }
  }

  return r;
}

int SlaterTotal(double *sd, double *se, int *j, int *ks, int k, int mode) {
  int t, kk, tt, maxn, minn;
  int tmin, tmax;
  double e, a, d, a1, a2, am, *v1, *v2, xd, xe;
  int err;
  int kl0, kl1, kl2, kl3;
  int k0, k1, k2, k3;
  int js[4];
  ORBITAL *orb0, *orb1, *orb2, *orb3;

#ifdef PERFORM_STATISTICS 
  clock_t start, stop;
  start = clock();
#endif

  k0 = ks[0];
  k1 = ks[1];
  k2 = ks[2];
  k3 = ks[3];
  kk = k/2;
  if (_slater_kmax >= 0 && kk > _slater_kmax) {
    if (sd) *sd = 0.0;
    if (se) *se = 0.0;
    return 0;
  }
  maxn = 0;
  minn = 1000000;
  orb0 = GetOrbitalSolved(k0);
  orb1 = GetOrbitalSolved(k1);
  orb2 = GetOrbitalSolved(k2);
  orb3 = GetOrbitalSolved(k3);

  int mode0 = mode;
  if (orb0->n <= 0) {
    maxn = -1;
  } else {
    if (orb0->n > maxn) {
      maxn = orb0->n;
    }
    if (orb0->n < minn) {
      minn = orb0->n;
    }
  }
  if (orb1->n <= 0) {
    maxn = -1;
  } else {
    if (orb1->n > maxn) {
      maxn = orb1->n;
    }
    if (orb1->n < minn) {
      minn = orb1->n;
    }
  }
  if (orb2->n <= 0) {
    maxn = -1;
  } else {
    if (orb2->n > maxn) {
      maxn = orb2->n;
    }
    if (orb2->n < minn) {
      minn = orb2->n;
    }
  }
  if (orb3->n <= 0) {
    maxn = -1;
  } else {
    if (orb3->n > maxn) {
      maxn = orb3->n;
    }
    if (orb3->n < minn) {
      minn = orb3->n;
    }
  }

  xd = 1.0;
  xe = 1.0;
  if (maxn > 0 && k0 == k2 && k1 == k3 &&
      k0 < MAXNSSC && k1 < MAXNSSC) {
    if (k < MAXKSSC) {
      xd = _slater_scale[k][k0][k1];
    }
    if (k+1 < MAXKSSC) {
      xe = _slater_scale[k+1][k0][k1];
    }
  }
  if (orb0->wfun == NULL || orb1->wfun == NULL ||
      orb2->wfun == NULL || orb3->wfun == NULL) {
    if (sd) *sd = 0.0;
    if (se) *se = 0.0;
    return 0;
  }

  kl0 = GetLFromKappa(orb0->kappa);
  kl1 = GetLFromKappa(orb1->kappa);
  kl2 = GetLFromKappa(orb2->kappa);
  kl3 = GetLFromKappa(orb3->kappa);
  if (orb1->n < 0 || orb3->n < 0) {
    mode = 2;
  } else {
    if (kl1 > slater_cut.kl0 && kl3 > slater_cut.kl0) {
      if (se) {
	*se = 0.0;
	se = NULL;
      }
    }
    if (kl0 > slater_cut.kl0 && kl2 > slater_cut.kl0) {
      if (se) {
	*se = 0.0;
	se = NULL;
      }
    }  
    if (kl1 > slater_cut.kl1 && kl3 > slater_cut.kl1) {
      mode = 2;
    }
    if (kl0 > slater_cut.kl1 && kl2 > slater_cut.kl1) {
      mode = 2;
    }
  }
  if (qed.br == 0 && IsOdd((kl0+kl1+kl2+kl3)/2)) {
    if (sd) *sd = 0.0;
    if (se) *se = 0.0;
    return 0;
  }

  if (j) {
    memcpy(js, j, sizeof(int)*4);
  } else {
    js[0] = 0;
    js[1] = 0;
    js[2] = 0;
    js[3] = 0;
  }

  if (js[0] <= 0) js[0] = GetJFromKappa(orb0->kappa);
  if (js[1] <= 0) js[1] = GetJFromKappa(orb1->kappa);
  if (js[2] <= 0) js[2] = GetJFromKappa(orb2->kappa);
  if (js[3] <= 0) js[3] = GetJFromKappa(orb3->kappa);  

  am = AMU * potential->atom->mass;
  if (sd) {
    d = 0.0;
    if (Triangle(js[0], js[2], k) && Triangle(js[1], js[3], k)) {
      if (kk == 1 && qed.sms && maxn > 0) {
	v1 = Vinti(k0, k2);
	v2 = Vinti(k1, k3);
      } else {
	v1 = NULL;
	v2 = NULL;
      }
      if (IsEven((kl0+kl2)/2+kk) && IsEven((kl1+kl3)/2+kk)) {	
	err = Slater(&d, k0, k1, k2, k3, kk, mode);
	d *= xd;
	if (v1 && v2) {
	  d -= (v1[0] + v1[2]) * v2[0]/am;
	}
      }
      a1 = ReducedCL(js[0], k, js[2]);
      a2 = ReducedCL(js[1], k, js[3]); 
      if (v1 && v2) {
	if (IsEven((kl1+kl3)/2+kk)) {
	  d -= v1[1]*v2[0]/am;
	  if (qed.sms == 3 &&
	      fabs(a1)>1e-10 &&
	      fabs(a2)>1e-10 &&
	      kl0 == kl2 && js[0] == js[2]) {
	    d += ((0.25*(kl1==kl3 && js[1]==js[3]))+0.5*a2) *
	      v1[2]*v2[2]/(am*a1*a2);
	  }
	} else {
	  if (qed.sms == 3 &&
	      fabs(a1)>1e-10 &&
	      fabs(a2)>1e-10 &&
	      kl0 == kl2 && js[0] == js[2] &&
	      kl1 == kl3 && js[1] == js[3]) {
	    d += 0.25*v1[2]*v2[2]/(am*a1*a2);
	  }
	}
	if (qed.sms == 3) {
	  d += 0.25*v1[1]*v2[1]/am;
	}
      }
      if (mode0 != -1 && (qed.br < 0 || (maxn > 0 && maxn <= qed.br))) {
	if (qed.minbr <= 0 || minn <= qed.minbr) {
	  int mbr = qed.mbr;
	  if (qed.nbr > 0 && (maxn <= 0 || maxn > qed.nbr)) mbr = 0;
	  double dbr = Breit(k0, k1, k2, k3, kk,
			     orb0->kappa, orb1->kappa, orb2->kappa, orb3->kappa,
			     kl0, kl1, kl2, kl3, mbr);
	  d += dbr;
	}
      }
      if (d) {
	d *= a1*a2;      
	if (k0 == k1 && k2 == k3) d *= 0.5;
      }
    }
    *sd = d;
  }
  
  if (!se) goto EXIT;

  if (abs(mode) == 2) {
    *se = 0.0;
    goto EXIT;
  }

  if (orb0->n == 0 || orb1->n== 0 || orb2->n == 0 || orb3->n == 0) {
    a = ZColl();
    d = MColl();
    if ((a && a != 1) || (d && d != 1)) {
      *se = 0;
      goto EXIT;
    }
  }
  *se = 0.0;
  if (k0 == k1 && (orb0->n > 0 || orb1->n > 0)) goto EXIT;
  if (k2 == k3 && (orb2->n > 0 || orb3->n > 0)) goto EXIT;
  tmin = abs(js[0] - js[3]);
  tt = abs(js[1] - js[2]);
  tmin = Max(tt, tmin);
  tmax = js[0] + js[3];
  tt = js[1] + js[2];
  tmax = Min(tt, tmax);
  tmax = Min(tmax, GetMaxRank());
  if (IsOdd(tmin)) tmin++;
  
  for (t = tmin; t <= tmax; t += 2) {
    a = W6j(js[0], js[2], k, js[1], js[3], t);
    if (fabs(a) > EPS30) {
      e = 0.0;
      if (t == 2 && qed.sms && maxn > 0) {
	v1 = Vinti(k0, k3);
	v2 = Vinti(k1, k2);
      } else {
	v1 = NULL;
	v2 = NULL;
      }
      if (IsEven((kl0+kl3+t)/2) && IsEven((kl1+kl2+t)/2)) {
	err = Slater(&e, k0, k1, k3, k2, t/2, mode);
	e *= xe;
	if (v1 && v2) {
	  e -= (v1[0]+v1[2])*v2[0]/am;
	}
      }
      a1 = ReducedCL(js[0], t, js[3]); 
      a2 = ReducedCL(js[1], t, js[2]);
      if (v1 && v2) {
	if (IsEven((kl1+kl2+t)/2)) {
	  e -= v1[2]*v2[0]/am;
	  if (qed.sms == 3 &&
	      fabs(a1)>1e-10 &&
	      fabs(a2)>1e-10 &&
	      kl0 == kl3 && js[0] == js[3]) {
	    e += ((0.25*(kl1==kl2&&js[1]==js[2]))+0.5*a2) *
	      v1[2]*v2[2]/(am*a1*a2);
	  }
	} else {
	  if (qed.sms == 3 &&
	      fabs(a1)>1e-10 &&
	      fabs(a2)>1e-10 &&
	      kl0 == kl3 && js[0] == js[3] &&
	      kl1 == kl2 && js[1] == js[2]) {
	    e += 0.25*v1[2]*v2[2]/(am*a1*a2);
	  }
	}
	if (qed.sms == 3) {
	  e += 0.25*v1[1]*v2[1]/am;
	}
      }
      if (mode0 != -1 && (qed.br < 0 || (maxn > 0 && maxn <= qed.br))) {
	if (qed.minbr <= 0 || minn <= qed.minbr) {
	  int mbr = qed.mbr;
	  if (qed.nbr > 0 && (maxn <= 0 || maxn > qed.nbr)) mbr = 0;
	  double ebr = Breit(k0, k1, k3, k2, t/2,
			     orb0->kappa, orb1->kappa, orb3->kappa, orb2->kappa,
			     kl0, kl1, kl3, kl2, mbr);
	  e += ebr;
	}
      }
      if (e) {
	e *= a * (k + 1.0) * a1 * a2;
	if (IsOdd(t/2 + kk)) e = -e;
	*se += e;
      }
    }
  }  
  
 EXIT:
#ifdef PERFORM_STATISTICS 
    stop = clock();
    rad_timing.radial_slater += stop - start;
#endif
  return 0;
}

double SelfEnergyRatio(ORBITAL *orb, ORBITAL *horb) {
  int i, k, m, npts;
  double *p, *q, e, z;
  double *large, *small;
  double a, b;
  
  if (orb->wfun == NULL) return 1.0;
  m = qed.mse%10;
  if (m == 2) {
    //modqed
    k = IdxVT(orb->kappa)-1;
    z = potential->atom->atomic_number;
    if (z < 10 || z > 120 || k < 0 || k >= NKSEP) {
      m = 1;
    } else {
      b = HydrogenicSelfEnergy(51, qed.pse, 1.0, potential, orb, orb);
      a = HydrogenicSelfEnergy(51, qed.pse, 1.0, potential, horb, horb);
      return b/a;
    }
  } else if (m == 3) {
    b = SelfEnergyExotic(orb);
    a = SelfEnergyExotic(horb);
    return b/a;
  }
  npts = potential->maxrp;
  p = Large(horb);
  q = Small(horb);
  large = Large(orb);
  small = Small(orb);
  for (i = 0; i < npts; i++) {
    if (m == 0) {
      //uehling potential
      a = potential->ZVP[i]/potential->rad[i];
    } else if (m == 1) {
      //welton scaling
      a = potential->qdist[i];
    } else {
      a = 1.0;
    }
    a *= potential->dr_drho[i];
    _dwork11[i] = (p[i]*p[i] + q[i]*q[i])*a;
    _dwork12[i] = (large[i]*large[i] + small[i]*small[i])*a;
  }
  if (i < 5) return 1.0;
  a = Simpson(_dwork11, 0, i-1);
  b = Simpson(_dwork12, 0, i-1);
  if (1+a == 1) return 1.0;
  return b/a;
}

ORBITAL *SolveAltOrbital(ORBITAL *orb, POTENTIAL *p) {
  ORBITAL *norb;
  int ierr;

  if (!p->hlike && !p->hpvs) {
    return orb;
  }
  norb = (ORBITAL *) malloc(sizeof(ORBITAL));
  InitOrbitalData(norb, 1);
  norb->n = orb->n;
  norb->kappa = orb->kappa;
  norb->energy = 0.0;
  if (p->nb > 0 && p->sturm_idx > 0 && orb->n > p->nb) {
    norb->energy = _sturm_eref;
  }
  p->flag = -1;
  ierr = RadialSolver(norb, p);
  if (ierr < 0) {
    MPrintf(-1, "error in SolveAltOrbital: %d %d %d\n",
	    ierr, norb->n, norb->kappa);
  }
  return norb;
}

//for exotic atoms (muonic e.g.), use leading order.
//Barrett, R.C., Phys.Lett. B28, 93 (1968)
double SelfEnergyExotic(ORBITAL *orb) {
  double a, a2, a3, p2, v2, x, y, c0, c1, b;
  int i, n, k, j2, k2;
  double *p, *q;
  
  a = FINE_STRUCTURE_CONST;
  a2 = FINE_STRUCTURE_CONST2;
  a3 = a2*a;

  c0 = a3/(3*PI);
  c1 = a3/(4*PI);
  GetJLFromKappa(orb->kappa, &j2, &k2);
  c1 *= (j2*(j2+2.0)-k2*(k2+2.0)-3.0)/8.0;
  
  n = orb->ilast;
  p = Large(orb);
  q = Small(orb);
  k = orb->kv;
  for (i = 0; i <= n; i++) {
    x = (p[i]*p[i] + q[i]*q[i])*potential->dr_drho[i];
    _dwork1[i] = x*potential->dVc[i]/potential->rad[i];
    _dwork2[i] = x*potential->qdist[i];
    y = orb->energy-potential->VT[k][i];
    _dwork3[i] = y*(y+2/a2)*x;
    y = potential->dVc[i];
    _dwork4[i] = y*y*x;    
  }
  c1 *= Simpson(_dwork1, 0, n);
  c0 *= Simpson(_dwork2, 0, n);
  p2 = Simpson(_dwork3, 0, n);
  v2 = Simpson(_dwork4, 0, n);
  b = 0.5*log(p2/(4*a2*v2));
  //b = log(1.0/(a2*fabs(orb->energy)));
  b += 0.833333;
  c0 *= b;
  //printf("see: %d %d %g %g %g %g %g %g %g\n", orb->n, orb->kappa, orb->energy, c0, c1, p2, v2, b, (c0+c1)*HARTREE_EV);
  return c0+c1;
}

double SelfEnergy(ORBITAL *orb1, ORBITAL *orb2) {
  double a, c;
  double an = 0.0;
  ORBITAL *orb0 = NULL;
  int msc = qed.mse%10;
  int ksc = qed.mse/10;

  
  if (qed.se == -1000000) return 0.0;
  if (orb1->n <= 0 || orb2->n <= 0) return 0.0;
  int nm = Min(orb1->n, orb2->n);
  if (orb1->energy > 0 && orb2->energy > 0) {
    return 0.0;
  } else {
    if (LEPTON_TYPE != 0) {
      if (orb1 != orb2) return 0.0;
      if (orb1->se < 0.999e31) return orb1->se;
      if (orb1->n <= 0) {
	orb1->se = 0.0;
	return 0.0;
      }
      if (!(qed.se < 0 || orb1->n <= qed.se)) {
	orb1->se = 0.0;
	return 0.0;
      }
      orb1->se = SelfEnergyExotic(orb1);
      return orb1->se;
    }
    double ae, ose, eb0;
    int idx, nb0;
    int kv = IdxVT(orb1->kappa);
    if (kv <= 0) return 0;
    int kv1 = kv-1;
    if (potential->ib > 0 && potential->nb > 0) {
      eb0 = fabs(Min(orb1->energy, orb2->energy));
      ae = qed.ose1;
      eb0 *= ae;
      ose = qed.ose0;
      if (qed.ase) {
	ose /= pow(nm, qed.ase);
      }
      nb0 = potential->nb+1;
      int nk = 1+GetLFromKappa(orb1->kappa)/2;
      if (nb0 < nk) nb0 = nk;
      for (;;nb0++) {
	if (potential->mpseudo > 0 && nb0 > potential->mpseudo) break;
	idx = OrbitalIndex(nb0, orb1->kappa, 0);
	orb0 = GetOrbitalSolved(idx);
	if (orb0->energy > eb0) break;
	if (orb0->rfn < potential->rfn[kv1]) break;
      }
      potential->nfn[kv1] = nb0;
      orb0 = NULL;
      if (orb1->n < nb0 && orb2->n >= nb0) {
	if (ose >= 0) {
	  an = eb0/orb2->energy;
	  an = Min(1.0, an);
	  an = ose>0?pow(an, ose):1.0;
	  if (orb2->n > nb0) {
	    int idx = OrbitalIndex(nb0, orb2->kappa, 0);
	    orb2 = GetOrbitalSolved(idx);
	  }
	  orb0 = orb1;
	} else {
	  if (orb2->rfn < potential->rfn[kv1] &&
	      orb2->energy > eb0) return 0.0;
	}
      } else if (orb1->n >= nb0 && orb2->n < nb0) {
	if (ose >= 0) {
	  an = eb0/orb1->energy;
	  an = Min(1.0, an);
	  an = ose>0?pow(an, ose):1.0;
	  if (orb1->n > nb0) {
	    int idx = OrbitalIndex(nb0, orb1->kappa, 0);
	    orb1 = GetOrbitalSolved(idx);
	  }
	  orb0 = orb2;
	} else {
	  if (orb1->rfn < potential->rfn[kv1] &&
	      orb1->energy > eb0) return 0.0;
	}
      }
    }
  }
  if (potential->ib > 0 &&
      (orb1->n > potential->nb || orb2->n > potential->nb)) {
    if (ksc < 4) return 0;
  }
  if (orb1 != orb2) {
    if (ksc < 6) return 0.0;
    if (orb1->n <= 0 || orb2->n <= 0) return 0.0;
    if (qed.se >= 0 && (orb1->n > qed.se || orb2->n > qed.se)) return 0.0;
    if (potential->nb > nm &&
	(orb1->n > potential->nb || orb2->n > potential->nb)) {
      c = potential->nb-nm;
      c = pow(c, qed.ise);
      c = 1 + qed.cse0*c/(1.0 + qed.cse1*c);
    } else {
      c = 0.0;
    }
    if (orb0 != NULL && orb0->ose < 1e30) {
      a = orb0->ose;
      if (an > 0) a *= an;
      if (c > 0) a *= c;
      return a;
    }
    if (orb1->rorb == NULL) {
      orb1->rorb = SolveAltOrbital(orb1, rpotential);
    }
    if (orb2->rorb == NULL) {
      orb2->rorb = SolveAltOrbital(orb2, rpotential);
    }
    a = HydrogenicSelfEnergy(qed.mse, qed.pse, c, potential,
			     orb1->rorb, orb2->rorb);
    if (orb0 != NULL) orb0->ose = a;
    if (an > 0) a *= an;
    if (c > 0) a *= c;
    return a;
  }  
  if (orb1->se < 0.999e31) return orb1->se;
  if (orb1->n <= 0) {
    orb1->se = 0.0;
    return 0.0;
  }
  if (!(qed.se < 0 || orb1->n <= qed.se)) {
    orb1->se = 0.0;
    return 0.0;
  }
  if (qed.sse > 0 && orb1->n > qed.sse) {
    int k = GetLFromKappa(orb1->kappa)/2;
    if (k < orb1->n-1) {
      int idx = OrbitalIndex(k+1, orb1->kappa, 0);
      ORBITAL *orb = GetOrbitalSolved(idx);
      a = SelfEnergy(orb, orb);
      c = SelfEnergyRatio(orb1, orb);
      orb1->se = a*c;
      if (qed.pse) {
	MPrintf(-1, "SE: z=%g, n=%d, kappa=%2d, e0=%11.4E, md=%d, screen=%11.4E, final=%11.4E\n", potential->atom->atomic_number, orb1->n, orb1->kappa, orb1->energy, qed.mse, c, orb1->se);
      }
      return orb1->se;
    }
  }
  if (orb1->rorb == NULL) {
    orb1->rorb = SolveAltOrbital(orb1, rpotential);
  }
  if (msc == 9 || fabs(potential->N-1)<1e-5) {
    c = 1.0;
  } else {
    if (orb1->horb == NULL) {
      orb1->horb = SolveAltOrbital(orb1, hpotential);
    }
    if (orb1->horb->wfun == NULL) {
      MPrintf(-1, "hlike orbital for SE screening null wfun: %d %d\n",
	      orb1->horb->n, orb1->horb->kappa);
      c = 1.0;
    }
    c = SelfEnergyRatio(orb1->rorb, orb1->horb);
  }
  orb1->se = HydrogenicSelfEnergy((qed.nms%10)*100+qed.mse, qed.pse, c, potential, orb1->rorb, NULL);
  return orb1->se;
}

double RadialNMS(ORBITAL *orb1, ORBITAL *orb2, int kv) {
  int i, k0, k1, nms;
  double a, r, r2, mass;
  
  if (qed.se == -1000000) return 0.0;  
  if (qed.nms <= 0) return 0.0;
  if (orb1->n <= 0 || orb2->n <= 0) return 0.0;

  nms = qed.nms%10;
  mass = AMU*potential->atom->mass;
  r = 0.0;
  k0 = orb1->idx;
  k1 = orb2->idx;
  if (qed.nms > 10 && k0 != k1) return r;
  
  if (nms == 1) {
    for (i = 0; i < potential->maxrp; i++) {
      _yk[i] = potential->VT[kv][i];
    }
    a = 0.0;
    Integrate(_yk, orb1, orb2, 1, &a, -1);
    a = -a;
    if (k0 == k1) a += orb1->energy;
    a /= mass;
    r = a;
    for (i = 0; i < potential->maxrp; i++) {
      _yk[i] = orb1->energy - potential->VT[kv][i];
      _yk[i] *= orb2->energy - potential->VT[kv][i];
    }
    Integrate(_yk, orb1, orb2, 1, &a, -1);
    a *= FINE_STRUCTURE_CONST2/(2.0 * mass);
    r += a;
  } else {
    double *p1, *p2, *q1, *q2, az;
    int m1, j2, k, kt;
    m1 = Min(orb1->ilast, orb2->ilast);
    p1 = Large(orb1);
    p2 = Large(orb2);
    q1 = Small(orb1);
    q2 = Small(orb2);
    GetJLFromKappa(orb1->kappa, &j2, &k);
    k /= 2;
    kt = j2-k;
    DrLargeSmall(orb1, potential, _dwork1, _dwork3);
    DrLargeSmall(orb2, potential, _dwork2, _dwork4);
    for (i = 0; i <= m1; i++) {
      _dwork[i] = _dwork1[i]*_dwork2[i];
      _dwork[i] += _dwork3[i]*_dwork4[i];
      r2 = potential->rad[i];
      r2 *= r2;
      a = p1[i]*p2[i]*k*(k+1.0) + q1[i]*q2[i]*kt*(kt+1.0);
      _dwork[i] += a/r2;
      if (nms > 2) {
	az = FINE_STRUCTURE_CONST*potential->VT[kv][i];
	a = 2*az*(q1[i]*_dwork2[i]+q2[i]*_dwork1[i]);
	_dwork[i] += a;
	a = az*(orb1->kappa-1)*(q1[i]*p2[i]+q2[i]*p1[i]);
	_dwork[i] += a/potential->rad[i];
      }
      _dwork[i] *= potential->dr_drho[i];
    }
    r = Simpson(_dwork, 0, m1);
    r /= 2*mass;
  }  
  if (qed.pse > 1) {
    MPrintf(-1, "NMS: %d %d %d %d %d %18.10E %18.10E\n", qed.nms, orb1->n, orb1->kappa, orb2->n, orb2->kappa, mass, r);
  }
  return r;
}

double QED1E(int k0, int k1) {
  int i, kv;
  ORBITAL *orb1, *orb2;
  int index[2];
  double *p, r, a, c;

  if (qed.se == -1000000) return 0.0;  
  if (qed.nms == 0 && qed.se == 0 && qed.vp%100 == 0) {
      return 0.0;
  }

  orb1 = GetOrbitalSolved(k0);
  orb2 = GetOrbitalSolved(k1);
  
  if (orb1->n <= 0 || orb2->n <= 0) {
    return 0.0;
  }
  
  kv = orb1->kv;
  if (kv < 0 || kv > NKSEP) {
    MPrintf(-1, "invalid orbital kv in QED1E: %d %d %d\n",
	    orb1->n, orb1->kappa, orb1->kv);
    return 0.0;
  }
  
  if (orb1->wfun == NULL || orb2->wfun == NULL) {
    return 0.0;
  }
  
  if (k0 > k1) {
    index[0] = k1;
    index[1] = k0;
  } else {
    index[0] = k0;
    index[1] = k1;
  }
  
  LOCK *lock = NULL;
  int myrank = MyRankMPI()+1;
  p = (double *) MultiSet(qed1e_array, index, NULL, &lock,
			  InitDoubleData, NULL);
  double pp;
#pragma omp atomic read
  pp = *p;
  int locked = 0;
  if (lock && !(p && pp)) {
    SetLock(lock);
    locked = 1;
#pragma omp atomic read
    pp = *p;
  }
  if (p && pp) {
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    qed1e_array->iset -= myrank;
#pragma omp flush
    return *p;
  }
  r = 0.0;
  if (orb1->kappa != orb2->kappa) {
    printf("dk: %d %d %d %d\n", orb1->n, orb1->kappa, orb2->n, orb2->kappa);
  }
  a = RadialNMS(orb1, orb2, kv);
  r += a;
  if (!potential->pvp && potential->mvp) {
    for (i = 0; i < potential->maxrp; i++) {
      _yk[i] = -potential->ZVP[i]/potential->rad[i];
    }
    Integrate(_yk, orb1, orb2, 1, &a, 0);
    if (qed.pse > 2) {
      MPrintf(-1, "VP: %d %d %d %d %18.10E\n", orb1->n, orb1->kappa, orb2->n, orb2->kappa, a);
    }
    r += a;      
  }
  if (optimize_control.mce < 20) {
    a = SelfEnergy(orb1, orb2);
    r += a;
    if (k0 == k1) {
      orb1->qed = r;
    }
  }
#pragma omp atomic write
  *p = r;
#pragma omp flush
  if (locked) ReleaseLock(lock);
#pragma omp atomic
  qed1e_array->iset -= myrank;
#pragma omp flush
  return r;
}

double *Vinti(int k0, int k1) {
  int i;
  ORBITAL *orb1, *orb2;
  int index[2];
  double **p, *large0, *small0, *large1, *small1;
  int ka0, ka1, m1;
  double a, b;

  if (qed.se == -1000000) {    
    return NULL;
  }

  orb1 = GetOrbitalSolved(k0);
  orb2 = GetOrbitalSolved(k1);

  if (orb1->n <= 0 || orb2->n <= 0) {
    return NULL;
  }
  if (orb1->wfun == NULL || orb2->wfun == NULL) {
    return NULL;
  }
  
  index[0] = k0;
  index[1] = k1;
  LOCK *lock = NULL;
  int myrank = MyRankMPI()+1;
  p = (double **) MultiSet(vinti_array, index, NULL, &lock,
			   InitPointerData, FreeMultipole);
  double *pp;
#pragma omp atomic read
  pp = *p;
  int locked = 0;
  if (lock && !pp) {
    SetLock(lock);
    locked = 1;
#pragma omp atomic read
    pp = *p;
  }
  if (pp) {
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    vinti_array->iset -= myrank;
#pragma omp flush
    return *p;
  }

  double *r;
  r = (double *) malloc(sizeof(double)*3);
  m1 = Min(orb1->ilast, orb2->ilast);
  ka0 = orb1->kappa;
  ka1 = orb2->kappa;
  large0 = Large(orb1);
  large1 = Large(orb2);
  small0 = Small(orb1);
  small1 = Small(orb2);
  a = 0.5*(ka0*(ka0+1.0) - ka1*(ka1+1.0));
  b = 0.5*(-ka0*(-ka0+1.0) + ka1*(-ka1+1.0));
  DrLargeSmall(orb2, potential, _dwork1, _dwork2);  
  for (i = 0; i <= m1; i++) {
    _yk[i] = large0[i]*_dwork1[i] - a*large0[i]*large1[i]/potential->rad[i];
    _yk[i] += small0[i]*_dwork2[i] - b*small0[i]*small1[i]/potential->rad[i];
    _yk[i] *= potential->dr_drho[i];
  }
  r[0] = Simpson(_yk, 0, m1);
  r[1] = r[2] = 0;  
  if (qed.sms == 1) {
#pragma omp atomic write
    *p = r;
#pragma omp flush
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    vinti_array->iset -= myrank;
#pragma omp flush
    return r;
  }

  int j0, j1, kl0, kl1, kt0, kt1, kv;
  double sd, sd1, sd2, az, ac;
  kv = orb1->kv;
  if (kv < 0 || kv > NKSEP) {
    MPrintf(-1, "invalid orbital kv in Vinti: %d %d %d\n",
	    orb1->n, orb1->kappa, orb1->kv);
    kv = 0;
  }
  GetJLFromKappa(orb1->kappa, &j0, &kl0);
  GetJLFromKappa(orb2->kappa, &j1, &kl1);
  kt0 = 2*j0-kl0;
  kt1 = 2*j1-kl1;
  r[1] = 0;
  if (kt0 == kl1 || kl0 == kt1) {
    ac = ReducedCL(j0, 2, j1);
    if (fabs(ac) > 1e-10) {
      for (i = 0; i < potential->maxrp; i++) {
	_yk[i] = 0;
      }
      sd = sqrt(6*(j0+1.0)*(j1+1.0));
      if (kt0 == kl1) {
	sd1 = sd*W6j(1, j0, kt0, j1, 1, 2);
	if (IsEven((kt0+1+j0)/2)) sd1 = -sd1;
	for (i = 0; i < potential->maxrp; i++) {
	  _yk[i] -= small0[i]*large1[i]*sd1;
	}
      }
      if (kl0 == kt1) {
	sd2 = sd*W6j(1, j0, kl0, j1, 1, 2);
	if (IsEven((kl0+1+j0)/2)) sd2 = -sd2;
	for (i = 0; i <= m1; i++) {
	  _yk[i] += small1[i]*large0[i]*sd2;
	}
      }    
      for (i = 0; i <= m1; i++) {
	az = FINE_STRUCTURE_CONST*potential->VT[kv][i];
	_yk[i] *= az*potential->dr_drho[i];
      }
      r[1] = -Simpson(_yk, 0, m1);
      r[1] /= ac;
    }
  }
  
  for (i = 0; i <= m1; i++) {
    _yk[i] = small0[i]*large1[i] - small1[i]*large0[i];
    az = FINE_STRUCTURE_CONST*potential->VT[kv][i];
    _yk[i] *= az*potential->dr_drho[i];
  }
  r[2] = -Simpson(_yk, 0, m1);
#pragma omp atomic write
  *p = r;
#pragma omp flush
  if(locked) ReleaseLock(lock);
#pragma omp atomic
  vinti_array->iset -= myrank;
#pragma omp flush
  return *p;
}

double BreitC(int n, int m, int k, int k0, int k1, int k2, int k3) {
  int ka0, ka1, ka2, ka3, kb, kp;
  double r, b, c;
  
  ka0 = GetOrbital(k0)->kappa;
  ka1 = GetOrbital(k1)->kappa;
  ka2 = GetOrbital(k2)->kappa;
  ka3 = GetOrbital(k3)->kappa;
  if (k == m) {
    r = -(ka0 + ka2)*(ka1 + ka3);
    if (r) r /= (m*(m+1.0));
  } else if (k == (m + 1)) {
    kb = ka2 - ka0;
    kp = ka3 - ka1;
    b = (m + 2.0)/(2.0*(2.0*m + 1.0));
    c = -(m - 1.0)/((2.0*m+1.0)*(2.0*m+2.0));
    switch (n) {
    case 0:
      r = (k + kb)*(b + c*kp);
      break;
    case 1:
      r = (k + kp)*(b + c*kb);
      break;
    case 2:
      r = (k - kb)*(b - c*kp);
      break;
    case 3:
      r = (k - kp)*(b - c*kb);
      break;
    case 4:
      r = -(k + kb)*(b - c*kp);
      break;
    case 5:
      r = -(k - kp)*(b + c*kb);
      break;
    case 6:
      r = -(k - kb)*(b + c*kp);
      break;
    case 7:
      r = -(k + kp)*(b - c*kb);
      break;
    default:
      r = 0;
    }
  } else {
    kb = ka2 - ka0;
    kp = ka3 - ka1;
    b = (m - 1.0)/(2.0*(2.0*m + 1.0));
    c = (m + 2.0)/(2.0*m*(2.0*m + 1.0));
    switch (n) {
    case 0:
      r = (b + c*kb)*(kp - k - 1.0);
      break;
    case 1:
      r = (b + c*kp)*(kb - k - 1.0);
      break;
    case 2:
      r = (b - c*kb)*(-kp - k - 1.0);
      break;
    case 3:
      r = (b - c*kp)*(-kb - k - 1.0);
      break;
    case 4:
      r = -(b + c*kb)*(-kp - k - 1.0);
      break;
    case 5:
      r = -(b - c*kp)*(kb - k - 1.0);
      break;
    case 6:
      r = -(b - c*kb)*(kp - k - 1.0);
      break;
    case 7:
      r = -(b + c*kp)*(-kb - k - 1.0);
      break;
    default:
      r = 0;
    }
  }

  return r;
}

int BreitSYK(int k0, int k1, int k, double *z) {
  ORBITAL *orb0, *orb1;
  int index[3];
  int myrank = MyRankMPI()+1;
  FLTARY *byk;
  int npts = 0;
  int i;
  
  byk = NULL;
  LOCK *xlock = NULL;
  int xlocked = 0;
  if (xbreit_array[4]->maxsize != 0) {
    index[0] = k0;
    index[1] = k1;
    index[2] = k;
    byk = (FLTARY *) MultiSet(xbreit_array[4], index, NULL, &xlock,
			      InitFltAryData, FreeFltAryData);
#pragma omp atomic read
    npts = byk->npts;
    if (xlock && npts <= 0) {
      SetLock(xlock);
      xlocked = 1;
#pragma omp atomic read
      npts = byk->npts;
    }
    if (npts > 0) {
      for (i = 0; i < byk->npts; i++) {
	z[i] = byk->yk[i];
      }
      for (; i < potential->maxrp; i++) z[i] = 0.0;
    }
  }
  if (byk == NULL || npts < 0) {
    orb0 = GetOrbitalSolved(k0);
    orb1 = GetOrbitalSolved(k1);
    for (i = 0; i < potential->maxrp; i++) {
      _dwork1[i] = pow(potential->rad[i], k);
    }    
    Integrate(_dwork1, orb0, orb1, -6, z, 0);    
    for (i = 0; i < potential->maxrp; i++) {
      if (z[i]) z[i] /= _dwork1[i]*potential->rad[i];
    }
    for (i = potential->maxrp-1; i >= 0; i--) {
      if (z[i]) break;
    }
    npts = i+1;
    if (byk != NULL) {
      int size = sizeof(float)*npts;
      byk->yk = malloc(size);
      AddMultiSize(xbreit_array[4], size);
      for (i = 0; i < npts; i++) {	
	byk->yk[i] = z[i];
      }
#pragma omp flush
#pragma omp atomic write
      byk->npts = npts;
#pragma omp flush
    }
  }
  if (xlocked) ReleaseLock(xlock);
  if (xbreit_array[4]->maxsize != 0) {
#pragma omp atomic
    xbreit_array[4]->iset -= myrank;
  }
#pragma omp flush
  return npts;
}

double BreitS(int k0, int k1, int k2, int k3, int k) {
  ORBITAL *orb2, *orb3;
  int index[5], i;
  double *p0, r;
  LOCK *lock = NULL;
  int locked = 0;
  int myrank = MyRankMPI()+1;
  if (breit_array->maxsize != 0) {
    index[0] = k0;
    index[1] = k1;
    index[2] = k2;
    index[3] = k3;
    index[4] = k;
    p0 = (double *) MultiSet(breit_array, index, NULL, &lock,
			     InitDoubleData, NULL);
    double pp;
#pragma omp atomic read
    pp = *p0;
    if (lock && !(p0 && pp)) {
      SetLock(lock);
      locked = 1;
#pragma omp atomic read
      pp = *p0;
    }
    if (p0 && pp) {
      r = pp;
      if (locked) ReleaseLock(lock);
#pragma omp atomic
      breit_array->iset -= myrank;
#pragma omp flush
      return r;
    }
  }
  double *z = _zk;
  int npts = BreitSYK(k0, k1, k, z);
  orb2 = GetOrbitalSolved(k2);
  orb3 = GetOrbitalSolved(k3);
  Integrate(z, orb2, orb3, 6, &r, 0);
  if (breit_array->maxsize != 0) {
    if (!r) r = 1e-100;
#pragma omp atomic write
    *p0 = r;
#pragma omp flush
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    breit_array->iset -= myrank;
  }
#pragma omp flush
  return r;
}

double BreitI(int n, int k0, int k1, int k2, int k3, int m) {
  double r;

  switch (n) {
  case 0:
    r = BreitS(k0, k2, k1, k3, m);
    break;
  case 1:
    r = BreitS(k1, k3, k0, k2, m);
    break;
  case 2:
    r = BreitS(k2, k0, k3, k1, m);
    break;
  case 3:
    r = BreitS(k3, k1, k2, k0, m);
    break;
  case 4:
    r = BreitS(k0, k2, k3, k1, m);
    break;
  case 5:
    r = BreitS(k3, k1, k0, k2, m);
    break;
  case 6:
    r = BreitS(k2, k0, k1, k3, m);
    break;
  case 7:
    r = BreitS(k1, k3, k2, k0, m);
    break;
  default:
    r = 0.0;
  }

  return r;
}

//mbr=2 uses freq-dep form. to calculate freq-indep values.
int BreitX(ORBITAL *orb0, ORBITAL *orb1, int k, int m, int w, int mbr,
	   double e, double *y) {
  int i, npts;
  double kf = 1.0;
  double x, r, b;
  int k2 = 2*k;
  int jy, k1, fd, rs;
  int index[3];  
  FLTARY *byk;

  if (y == NULL) y = _xk;
  if (e < 0) e = fabs(orb0->energy-orb1->energy);
  byk = NULL;
  LOCK *lock = NULL;
  int locked = 0;
  int myrank = MyRankMPI()+1;
  if ((mbr == 2 || (m < 3 && w == 0) || (m == 3 && w == 1))
      && xbreit_array[m]->maxsize != 0) {
    index[0] = orb0->idx;
    index[1] = orb1->idx;
    index[2] = k;
    byk = (FLTARY *) MultiSet(xbreit_array[m], index, NULL, &lock,
			      InitFltAryData, FreeFltAryData);
#pragma omp atomic read
    npts = byk->npts;
    if (lock && npts <= 0) {
      SetLock(lock);
      locked = 1;
#pragma omp atomic read
      npts = byk->npts;
    }
    if (npts > 0) {
      for (i = 0; i < byk->npts; i++) {
	if (e > 0) {
	  _dwork1[i] = FINE_STRUCTURE_CONST*e*potential->rad[i];
	} else {
	  _dwork1[i] = 0;
	}
	_dwork2[i] = pow(potential->rad[i], k);
	y[i] = byk->yk[i];
      }
      if (locked) ReleaseLock(lock);
#pragma omp atomic
      xbreit_array[m]->iset -= myrank;
#pragma omp flush
      return npts;
    }     
  }

  for (i = 1; i < k2; i += 2) {
    kf *= i;
  }
  double ef = FINE_STRUCTURE_CONST*e;
  double efk = pow(ef, k);

  for (i = 0; i < potential->maxrp; i++) {    
    if (e > 0) {
      x = FINE_STRUCTURE_CONST*e*potential->rad[i];
      _dwork1[i] = x;
    } else {
      x = 0;
      _dwork1[i] = 0;
    }
    _dwork2[i] = pow(potential->rad[i], k);
    switch (m) {
    case 0:
      if (x < qed.xbr) {
	_dwork[i] = (1 - 0.5*x*x/(k2+3.0))*_dwork2[i];
      } else {
	jy = 1;
	_dwork[i] = BESLJN(jy, k, x);
	_dwork[i] *= kf*(k2+1.0)/efk;	 
      }
      break;
    case 1:
      _dwork[i] = _dwork2[i]/potential->rad[i];
      break;
    case 2:
      if (x < qed.xbr) {
	_dwork[i] = 0.5/(1.0+k2);
      } else {
	jy = 3;
	k1 = k - 1;
	_dwork[i] = BESLJN(jy, k1, x);	
      }
      b = _dwork2[i]*potential->rad[i];
      _dwork[i] *= b;
      break;
    case 3:
      if (x < qed.xbr) {
	_dwork[i] = (1 - 0.5*x*x/(5+k2))*_dwork2[i]*potential->rad[i];
      } else {
	jy = 1;
	k1 = k + 1;
	_dwork[i] = BESLJN(jy, k1, x);
	_dwork[i] *= kf*(k2+1.0)*(k2+3.0)/(efk*ef);
      }
      break;
    default:
      _dwork[i] = 0;
      break;
    }
  }

  Integrate(_dwork, orb0, orb1, -6, y, 0);
  switch (m) {
  case 0:
    for (i = 0; i < potential->maxrp; i++) {
      y[i] /= _dwork2[i]*potential->rad[i];
    }
    break;
  case 1:
    for (i = 0; i < potential->maxrp; i++) {
      y[i] /= _dwork2[i];
    }
    break;
  case 2:
  case 3:
    for (i = 0; i < potential->maxrp; i++) {
      y[i] /= _dwork2[i]*potential->rad[i]*potential->rad[i];
    }
    break;
  }
  for (i = potential->maxrp-1; i >= 0; i--) {
    if (y[i]) break;
  }
  npts = i+1;
  if (byk) {
    int size = sizeof(float)*npts;
    byk->yk = malloc(size);
    AddMultiSize(xbreit_array[m], size);
    for (i = 0; i < npts; i++) {
      byk->yk[i] = y[i];
    }
#pragma omp flush
#pragma omp atomic write
    byk->npts = npts;
#pragma omp flush
#pragma omp atomic
    xbreit_array[m]->iset -= myrank;
  }
  if (locked) ReleaseLock(lock);
#pragma omp flush
  return npts;
}

double BreitRW(int k0, int k1, int k2, int k3, int k, int w, int mbr) {
  double e, x, r, b, kf;
  ORBITAL *orb0, *orb1, *orb2, *orb3;
  int maxrp, i, jy, kd;
  
  orb0 = GetOrbital(k0);
  orb1 = GetOrbital(k1);
  orb2 = GetOrbital(k2);
  orb3 = GetOrbital(k3);
  if (mbr == 2) {
    e = 0;
  } else {
    if (w == 0) {
      e = fabs(orb0->energy - orb2->energy);
    } else {
      e = fabs(orb1->energy - orb3->energy);
    }
  }
  double ef = FINE_STRUCTURE_CONST*e;
  double efk = pow(ef, k);
  kf = 1;
  kd = 2*k;
  for (i = 1; i < kd; i += 2) {
    kf *= i;
  }
  int npts = BreitX(orb0, orb2, k, 0, w, mbr, e, _xk);
  for (i = 0; i < npts; i++) {
    x = _dwork1[i];
    if (x < qed.xbr) {
      b = 1 - 0.5*x*x/(1.0-kd);
    } else {
      jy = 2;
      b = -BESLJN(jy, k, x)*(_dwork2[i]*potential->rad[i]*efk*ef)/kf;
    }
    _xk[i] *= b;
  }
  for (; i < potential->maxrp; i++) _xk[i] = 0.0;
  Integrate(_xk, orb1, orb3, 6, &r, 0);
  return r;
}

double BreitRK(int k0, int k1, int k2, int k3, int k, int mbr) {
  double r1, r2, r3, r4;
  r1 = BreitRW(k0, k1, k2, k3, k, 0, mbr);
  if ((k0 == k1 && k2 == k3) || (k0 == k3 && k2 == k1)) {
    r2 = r1;
  } else {
    r2 = BreitRW(k0, k1, k2, k3, k, 1, mbr);
  }
  if (k0 == k1 && k2 == k3) {
    r3 = r1;
  } else {
    r3 = BreitRW(k1, k0, k3, k2, k, 0, mbr);
  }
  if ((k0 == k1 && k2 == k3) || (k0 == k3 && k2 == k1)) {
    r4 = r3;
  } else {
    r4 = BreitRW(k1, k0, k3, k2, k, 1, mbr);
  }
  return 0.5*(r1+r2+r3+r4);
}

double BreitSW(int k0, int k1, int k2, int k3, int k, int w, int mbr) {  
  double e, x, xk, r, b, kf, s1, s2;
  ORBITAL *orb0, *orb1, *orb2, *orb3;
  int i, jy, kd, kj;
  
  orb0 = GetOrbital(k0);
  orb1 = GetOrbital(k1);
  orb2 = GetOrbital(k2);
  orb3 = GetOrbital(k3);
  if (mbr == 2) {
    e = 0.0;
  } else {
    if (w == 0) {
      e = fabs(orb0->energy - orb2->energy);
    } else {
      e = fabs(orb1->energy - orb3->energy);
    }
  }
  double ef = FINE_STRUCTURE_CONST*e;
  double efk = pow(ef, k);
  kf = 1;
  kd = 2*k;
  for (i = 1; i < kd; i += 2) {
    kf *= i;
  }

  int npts1 = BreitX(orb0, orb2, k, 1, w, mbr, e, _dwork12);
  int npts2 = BreitX(orb0, orb2, k, 2, w, mbr, e, _dwork13);
  int npts = Min(npts1, npts2);
  for (i = 0; i < npts; i++) {
    x = _dwork1[i];
    xk = _dwork2[i]*efk;
    if (x < qed.xbr) {
      b = -0.5/(1+kd);
    } else {
      jy = 4;
      kj = k + 1;
      b = BESLJN(jy, kj, x);
    }
    _dwork12[i] = _dwork12[i]*b;
    _dwork13[i] = _dwork13[i]*(1-x*x*b);
    _yk[i] = _dwork12[i] + _dwork13[i];
    b = (kd+1);
    _yk[i] *= b*b;
  }
  for (; i < potential->maxrp; i++) _yk[i] = 0.0;
  Integrate(_yk, orb1, orb3, 6, &s1, 0);
  if (k >= 0) {
    npts = BreitX(orb1, orb3, k, 3, w, mbr, e, _dwork12);
    for (i = 0; i < npts; i++) {
      x = _dwork1[i];
      if (x < qed.xbr) {
	b = 1 - 0.5*x*x/(3.0-kd);
      } else {
	jy = 2;
	kj = k-1;
	xk = _dwork2[i]*efk;
	b = -BESLJN(jy, kj, x)*xk*(kd-1.0)/kf;
      }
      _dwork12[i] *= b;    
      b = x*x/((kd-1.0)*(kd+3));
      _dwork12[i] *= b;
    }
    for (; i < potential->maxrp; i++) _dwork12[i] = 0;
    Integrate(_dwork12, orb0, orb2, 6, &s2, 0);
  } else {
    s2 = 0;
  }  
  r = s1 - s2;
  return r;
}

double BreitSK(int k0, int k1, int k2, int k3, int k, int mbr) {
  double r1, r2;
  r1 = BreitSW(k0, k1, k2, k3, k, 0, mbr);
  if ((k0 == k1 && k2 == k3) || (k0 == k3 && k1 == k2)) {
    r2 = r1;
  } else {      
    r2 = BreitSW(k0, k1, k2, k3, k, 1, mbr);
  }
  return 0.5*(r1+r2);
}

double BreitWW(int k0, int k1, int k2, int k3, int k,
	       int kp0, int kp1, int kp2, int kp3,
	       int kl0, int kl1, int kl2, int kl3, int mbr) {
  int m, m0, m1, n, ka, kap, kd;
  double a, b, c, r, c1, c2, c3, c4;

  if (k <= 0) return 0;

  int index[5];
  double *p;
  LOCK *lock = NULL;
  int locked = 0;
  int myrank = MyRankMPI()+1;
  if (wbreit_array->maxsize != 0) {
    index[0] = k0;
    index[1] = k1;
    index[2] = k2;
    index[3] = k3;
    index[4] = k;
    p = (double *) MultiSet(wbreit_array, index, NULL, &lock,
			    InitDoubleData, NULL);
    double pp;
#pragma omp atomic read
    pp = *p;
    if (lock && !(p && pp)) {
      SetLock(lock);
      locked = 1;
#pragma omp atomic read
      pp = *p;
    }
    if (p && pp) {
      r = pp;
      if (locked) ReleaseLock(lock);
#pragma omp atomic
      wbreit_array->iset -= myrank;
#pragma omp flush
      return r;
    }
  }
  m0 = k - 1;
  if (m0 < 0) m0 = 0;
  m1 = k + 1;
  kd = 2*k;
  ka = kp2 - kp0;
  kap = kp3 - kp1;
  r = 0.0;  
  for (m = m0; m <= m1; m++) {
    if (IsEven((kl0+kl2)/2 + m) || IsEven((kl1+kl3)/2 + m)) continue;
    if (m < k) {
      a = (k+1.0)/(k*(kd-1.0)*(kd+1.0));
      c1 = a*(ka+k)*(kap+k);
      c2 = a*(ka-k)*(kap-k);
      c3 = a*(ka+k)*(kap-k);
      c4 = a*(ka-k)*(kap+k);
    } else if (m == k) {
      a = -(kp0 + kp2)*(kp1 + kp3)/(k*(k+1.0));
      c1 = c2 = c3 = c4 = a;
    } else {
      a = k/((k+1.0)*(kd+1.0)*(kd+3.0));
      c1 = a*(ka-k-1)*(kap-k-1);
      c2 = a*(ka+k+1)*(kap+k+1);
      c3 = a*(ka-k-1)*(kap+k+1);
      c4 = a*(ka+k+1)*(kap-k-1);
    }

    if (fabs(c1) > 1e-30) {
      a = BreitRK(k0, k1, k2, k3, m, mbr);
      r += a*c1;
      //printf("rk1: %d %d %d %d %d %d %g %g %g %d %d %d %d %d %d %d %d %d %d %d\n", k0, k1, k2, k3, k, m, a, c1, r, kl0, kl1, kl2, kl3, kp0, kp1, kp2, kp3, ka, kap, kd);
    }

    if (fabs(c2) > 1e-30) {
      a = BreitRK(k2, k3, k0, k1, m, mbr);
      r += a*c2;
      //printf("rk2: %d %d %d %d %d %d %g %g %g\n", k0, k1, k2, k3, k, m, a, c2, r);
    }

    if (fabs(c3) > 1e-30) {
      a = BreitRK(k0, k3, k2, k1, m, mbr);
      r += a*c3;
      //printf("rk3: %d %d %d %d %d %d %g %g %g\n", k0, k1, k2, k3, k, m, a, c3, r);
    }

    if (fabs(c4) > 1e-30) {
      a = BreitRK(k2, k1, k0, k3, m, mbr);
      r += a*c4;
      //printf("rk4: %d %d %d %d %d %d %g %g %g\n", k0, k1, k2, k3, k, m, a, c4, r);
    }
  }
  
  if (k >= 0 && IsEven((kl0+kl2)/2+k) && IsEven((kl1+kl3)/2+k)) {
    b = 1.0/((kd+1.0)*(kd+1.0));
    c = b*(ka+k)*(kap-k-1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k0, k1, k2, k3, k, mbr);      
      r += a*c;
      //printf("sk1: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
    
    c = b*(kap+k)*(ka-k-1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k1, k0, k3, k2, k, mbr);
      r += a*c;
      //printf("sk2: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
    
    c = b*(ka-k)*(kap+k+1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k2, k3, k0, k1, k, mbr);
      r += a*c;
      //printf("sk3: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
    
    c = b*(kap-k)*(ka+k+1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k3, k2, k1, k0, k, mbr);
      r += a*c;
      //printf("sk4: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
    
    c = b*(ka+k)*(kap+k+1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k0, k3, k2, k1, k, mbr);
      r += a*c;
      //printf("sk5: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
    
    c = b*(kap-k)*(ka-k-1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k3, k0, k1, k2, k, mbr);
      r += a*c;
      //printf("sk6: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
    
    c = b*(ka-k)*(kap-k-1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k2, k1, k0, k3, k, mbr);
      r += a*c;
      //printf("sk7: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
    
    c = b*(kap+k)*(ka+k+1.0);
    if (fabs(c) > 1e-30) {
      a = BreitSK(k1, k2, k3, k0, k, mbr);
      r += a*c;
      //printf("sk8: %d %d %d %d %d %g %g %g %g\n", k0, k1, k2, k3, k, b, a, c, r);
    }
  }  
  if (wbreit_array->maxsize != 0) {
    if (!r) r = 1e-100;
#pragma omp atomic write
    *p = r;
#pragma omp flush
    if (locked) ReleaseLock(lock);
#pragma omp atomic
    wbreit_array->iset -= myrank;
  }
#pragma omp flush
  return r;
}

double BreitNW(int k0, int k1, int k2, int k3, int k,
	       int kl0, int kl1, int kl2, int kl3) {
  int m, m0, m1, n;
  double a, c, r;

  m0 = k - 1;
  if (m0 < 0) m0 = 0;
  m1 = k + 1;
  r = 0.0;
  for (m = m0; m <= m1; m++) {
    if (IsEven((kl0+kl2)/2 + m) || IsEven((kl1+kl3)/2 + m)) continue;
    for (n = 0; n < 8; n++) {
      c = BreitC(n, m, k, k0, k1, k2, k3);
      if (c) {
	a = BreitI(n, k0, k1, k2, k3, m);
	r += a*c;
      }
    }
  }

  return r;
}

double Breit(int k0, int k1, int k2, int k3, int k,
	     int kp0, int kp1, int kp2, int kp3,
	     int kl0, int kl1, int kl2, int kl3, int mbr) {
  double r;
  if (k <= 0) return 0;
  if (mbr == 0) {
    r = BreitNW(k0, k1, k2, k3, k, kl0, kl1, kl2, kl3);
  } else {
    r = BreitWW(k0, k1, k2, k3, k, kp0, kp1, kp2, kp3, kl0, kl1, kl2, kl3, mbr);
  }
  //printf("bri: %d %d %d %d %d %g %d %d %d %d\n", k0, k1, k2, k3, k, r, kp0, kp1, kp2, kp3);
  return r;
}

/* calculate the slater integral of rank k */
int Slater(double *s, int k0, int k1, int k2, int k3, int k, int mode) {
  int index[5];
  double *p;
  int ilast, i, npts, m;
  ORBITAL *orb0, *orb1, *orb2, *orb3;
  double norm;
#ifdef PERFORM_STATISTICS
  clock_t start, stop; 
  start = clock();
#endif

  index[0] = k0;
  index[1] = k1;
  index[2] = k2;
  index[3] = k3;
  index[4] = k;  

  LOCK *lock = NULL;
  int locked = 0;
  int myrank = MyRankMPI()+1;
  double pp = 0.0;
  if (abs(mode) < 2) {
    SortSlaterKey(index);
    p = (double *) MultiSet(slater_array, index, NULL, &lock,
			    InitDoubleData, NULL);
#pragma omp atomic read
    pp = *p;
    if (lock && !(p && pp)) {
      SetLock(lock);
      locked = 1;
#pragma omp atomic read
      pp = *p;
    }
  } else {
    p = NULL;
    pp = 0.0;
  }
  if (p && pp) {
    *s = pp;
  } else {
    orb0 = GetOrbitalSolved(k0);
    orb1 = GetOrbitalSolved(k1);
    orb2 = GetOrbitalSolved(k2);
    orb3 = GetOrbitalSolved(k3);
    *s = 0.0; 
    npts = potential->maxrp;
    switch (mode) {
    case 0: /* fall through to case 1 */
    case 1: /* full relativistic with distorted free orbitals */
      GetYk(k, _yk, orb0, orb2, k0, k2, -1); 
      if (orb1->n > 0) ilast = orb1->ilast;
      else ilast = npts-1;
      if (orb3->n > 0) ilast = Min(ilast, orb3->ilast);
      for (i = 0; i <= ilast; i++) {
	_yk[i] = (_yk[i]/potential->rad[i]);
      }
      Integrate(_yk, orb1, orb3, 1, s, 0);
      break;
    
    case -1: /* quasi relativistic with distorted free orbitals */
      GetYk(k, _yk, orb0, orb2, k0, k2, -2);
      if (orb1->n > 0) ilast = orb1->ilast;
      else ilast = npts-1;
      if (orb3->n > 0) ilast = Min(ilast, orb3->ilast);
      for (i = 0; i <= ilast; i++) {
	_yk[i] /= potential->rad[i];
      }     
      Integrate(_yk, orb1, orb3, 2, s, 0);

      norm  = orb0->qr_norm;
      norm *= orb1->qr_norm;
      norm *= orb2->qr_norm;
      norm *= orb3->qr_norm;
      *s *= norm;
      break;

    case 2: /* separable coulomb interaction, orb0, orb2 is inner part */
      m = k;
      *s = RadialMoments(m, k0, k2);
      if (*s != 0.0) {
	m = -m-1;
	*s *= RadialMoments(m, k1, k3);
      }
      break;

    case -2: /* separable coulomb interaction, orb1, orb3 is inner part  */
      m = k;
      *s = RadialMoments(m, k1, k3);
      if (*s != 0.0) {
	m = -m-1;
	*s *= RadialMoments(m, k0, k2);      
      }
      break;
      
    default:
      break;
    }      
    if (p) {
#pragma omp atomic write
      *p = *s;
#pragma omp flush
    }
  }
  if (locked) ReleaseLock(lock);
  if (p) {
#pragma omp atomic
    slater_array->iset -= myrank;
  }
#pragma omp flush
#ifdef PERFORM_STATISTICS 
    stop = clock();
    rad_timing.radial_2e += stop - start;
#endif
  return 0;
}

/* reorder the orbital index appears in the slater integral, so that it is
   in a form: a <= b <= d, a <= c, and if (a == b), c <= d. */ 
void SortSlaterKey(int *kd) {
  int i;

  if (kd[0] > kd[2]) {
    i = kd[0];
    kd[0] = kd[2];
    kd[2] = i;
  }

  if (kd[1] > kd[3]) {
    i = kd[1];
    kd[1] = kd[3];
    kd[3] = i;
  }
  
  if (kd[0] > kd[1]) {
    i = kd[0];
    kd[0] = kd[1];
    kd[1] = i;
    i = kd[2];
    kd[2] = kd[3];
    kd[3] = i;
  } else if (kd[0] == kd[1]) {
    if (kd[2] > kd[3]) {
      i = kd[2];
      kd[2] = kd[3];
      kd[3] = i;
    }
  }
}

void PrepYK(int n, int m) {
#pragma omp parallel default(shared)
  {
    ORBITAL *orb0, *orb1;
    int i, j, kk, k, kmax, i0, i1, j0, j1, k0, k1, mk;
    double wt = WallTime();
    int nyk = 0;
    
    kmax = GetMaxRank();
    i0 = 0;
    i1 = GetNumBounds()-1;
    for (kk = 0; kk <= kmax; kk += 2) {
      k = kk/2;
      for (i = i0; i <= i1; i++) {
	orb0 = GetOrbital(i);
	GetJLFromKappa(orb0->kappa, &j0, &k0);
	for (j = i0; j <= i1; j++) {
	  orb1 = GetOrbital(j);
	  if ((orb0->n < 0 || orb0->n > n) &&
	      (orb1->n < 0 || orb1->n > n)) continue;
	  GetJLFromKappa(orb1->kappa, &j1, &k1);
	  if (k0 > slater_cut.kl0 || k1 > slater_cut.kl0) continue;
	  if (!Triangle(j0, j1, kk)) continue;
	  if (m == 0) {
	    if (j < i) continue;
	    if (IsOdd(k0+k1)/2+k) continue;
	  }
	  if (m == 0) {
	    int skip = SkipMPI();
	    if (skip) continue;
	    GetYk(k, _yk, orb0, orb1, i, j, -1);
	    nyk++;
	  } else {
	    if (qed.br > 0 &&
		((orb0->n < 0 || orb0->n > qed.br) ||
		 (orb1->n < 0 || orb1->n > qed.br))) {
	      continue;
	    }
	    for (mk = k-1; mk <= k+1; mk++) {
	      if (mk < 0) continue;
	      if (IsEven((k0+k1)/2+mk)) continue;
	      int skip = SkipMPI();
	      if (skip) continue;
	      BreitSYK(i, j, mk, _zk);
	      nyk++;
	    }
	  }
	}
      }
    }
    MPrintf(-1, "PrepYk: %5d %5d %1d %3d %6d %12.5E %12.5E\n",
	    i0, i1, m, qed.br, nyk, WallTime()-wt, TotalSize());
  }
}

void PrepSlater(int ib0, int iu0, int ib1, int iu1,
		int ib2, int iu2, int ib3, int iu3) {
#pragma omp parallel default(shared)
  {
  int k, kmax, kk, i, j, p, q, m, ilast;
  int j0, j1, j2, j3, k0, k1, k2, k3;
  int index[6];
  double *dp;
  ORBITAL *orb0, *orb1, *orb2, *orb3;
  int c = 0;

  int myrank = MyRankMPI()+1;
  double wt0 = WallTime();
  kmax = GetMaxRank();
  for (kk = 0; kk <= kmax; kk += 2) {
    k = kk/2;
    for (i = ib0; i <= iu0; i++) {
      orb0 = GetOrbital(i);
      GetJLFromKappa(orb0->kappa, &j0, &k0);
      for (p = ib2; p <= iu2; p++) {
	if (p < i) continue;
	orb2 = GetOrbital(p);
	GetJLFromKappa(orb2->kappa, &j2, &k2);
	if (k0 > slater_cut.kl0 || k2 > slater_cut.kl0) continue;
	int skip = SkipMPI();
	if (skip) continue;	     
	GetYk(k, _yk, orb0, orb2, i, p, -1);
	ilast = potential->maxrp-1;
	for (m = 0; m <= ilast; m++) {
	  _yk[m] /= potential->rad[m];
	}
	for (j = ib1; j <= iu1; j++) {
	  if (j < i) continue;
	  orb1 = GetOrbital(j);
	  GetJLFromKappa(orb1->kappa, &j1, &k1);
	  for (q = ib3; q <= iu3; q++) {
	    if (q < j) continue;
	    orb3 = GetOrbital(q);
	    GetJLFromKappa(orb3->kappa, &j3, &k3);
	    if (i == j && q < p) continue;
	    if (IsOdd((k0+k2)/2+k) || 
		IsOdd((k1+k3)/2+k) ||
		!Triangle(j0, j2, kk) ||
		!Triangle(j1, j3, kk)) continue;
	    index[0] = i;
	    index[1] = j;
	    index[2] = p;
	    index[3] = q;
	    index[4] = k;
	    index[5] = 0;
	    LOCK *lock = NULL;
	    dp = MultiSet(slater_array, index, NULL, &lock,
			  InitDoubleData, NULL);
	    c++;
	    //if (lock) SetLock(lock);
	    double dpp;
#pragma omp atomic read
	    dpp = *dp;
	    if (dpp == 0) {
	      Integrate(_yk, orb1, orb3, 1, dp, 0);
	    }
	    //if (lock) ReleaseLock(lock);
#pragma omp atomic
	    slater_array->iset -= myrank;
	  }
	}
      }
    }
  }
  double wt1 = WallTime();
  MPrintf(-1, "PrepSlater: %d %g\n", c, wt1-wt0);
  }
}
      
int GetYk0(int k, double *yk, ORBITAL *orb1, ORBITAL *orb2, int type) {
  int i, ilast, i0;
  double a, max;

  for (i = 0; i < potential->maxrp; i++) {
    _dwork1[i] = pow(potential->rad[i], k);
  }
  Integrate(_dwork1, orb1, orb2, type, _zk, 0);
  for (i = 0; i < potential->maxrp; i++) {
    _zk[i] /= _dwork1[i];
    yk[i] = _zk[i];
    _zk[i] = _dwork1[i];
  }
  if (k > 2) {
    max = 0.0;
    for (i = 0; i < potential->maxrp; i++) {
      a = fabs(yk[i]);
      if (max < a) max = a;
    }
    max *= 1E-3;
    ilast = Min(orb1->ilast, orb2->ilast);
    for (i = 0; i < ilast; i++) {
      a = Large(orb1)[i]*Large(orb2)[i]*potential->rad[i];
      if (fabs(a) > max) break;
      _dwork1[i] = 0.0;
    }
    i0 = i;
  } else i0 = 0;
  for (i = i0; i < potential->maxrp; i++) {
    _dwork1[i] = pow(potential->rad[i0]/potential->rad[i], k+1);
  }
  Integrate(_dwork1, orb1, orb2, type, _xk, 0);
  ilast = potential->maxrp - 1;    
  for (i = i0; i < potential->maxrp; i++) {
    _xk[i] = (_xk[ilast] - _xk[i])/_dwork1[i];
    yk[i] += _xk[i];
  }

  return 0;
}  

/*
** this is a better version of Yk than GetYk0.
** note that on exit, _zk consists r^k, which is used in GetYk
*/      
int GetYk1(int k, double *yk, ORBITAL *orb1, ORBITAL *orb2, int type) {
  int i, ilast;
  double r0, a;
  
  ilast = Min(orb1->ilast, orb2->ilast);
  r0 = sqrt(potential->rad[0]*potential->rad[ilast]);  
  for (i = 0; i < potential->maxrp; i++) {
    _dwork1[i] = pow(potential->rad[i]/r0, k);
  }
  Integrate(_dwork1, orb1, orb2, type, _zk, 0);
  a = pow(r0, k);
  for (i = 0; i < potential->maxrp; i++) {
    _zk[i] /= _dwork1[i];
    yk[i] = _zk[i];
    _zk[i] = _dwork1[i]*a;
  }  
  for (i = 0; i < potential->maxrp; i++) {
    _dwork1[i] = (r0/potential->rad[i])/_dwork1[i];
  }
  Integrate(_dwork1, orb1, orb2, type, _xk, -1);
  for (i = 0; i < potential->maxrp; i++) {
    yk[i] += _xk[i]/_dwork1[i];
  }
      
  return 0;
}
      
int GetYk(int k, double *yk, ORBITAL *orb1, ORBITAL *orb2, 
	  int k1, int k2, int type) {
  int i, i0, i1, n, npts, ic0, ic1;
  double a, b, a2, b2, max, max1;
  int index[3];
  FLTARY *syk;

  syk = NULL;
  LOCK *lock = NULL;
  int locked = 0;
  int myrank = MyRankMPI()+1;
  if (yk_array->maxsize != 0) {
    if (k1 <= k2) {
      index[0] = k1;
      index[1] = k2;
    } else {
      index[0] = k2;
      index[1] = k1;
    }
    index[2] = k;
    syk = (FLTARY *) MultiSet(yk_array, index, NULL, &lock,
			      InitFltAryData, FreeFltAryData);
#pragma omp atomic read
    npts = syk->npts;
    if (lock && npts <= 0) {
      SetLock(lock);
      locked = 1;
#pragma omp atomic read
      npts = syk->npts;
    }
    if (npts > 0) {
      npts = npts-2;
      ic0 = npts;
      ic1 = npts+1;
      i0 = npts-1;
      for (i = npts-1; i < potential->maxrp; i++) {
	_dwork1[i] = pow(potential->rad[i0]/potential->rad[i], k);
      }
      for (i = 0; i < npts; i++) {
	yk[i] = syk->yk[i];
      }
      a = syk->yk[i0]/_dwork1[i0];
      for (i = npts; i < potential->maxrp; i++) {
	b = potential->rad[i] - potential->rad[i0];
	b = syk->yk[ic1]*b;
	if (b < -20) {
	  yk[i] = syk->yk[ic0];
	} else {
	  yk[i] = (a - syk->yk[ic0])*exp(b);
	  yk[i] += syk->yk[ic0];
	}
	yk[i] *= _dwork1[i];
      }    
    }
  }
  if (syk == NULL || npts <= 0) {
    GetYk1(k, yk, orb1, orb2, type);
    max = 0;    
    for (i = 0; i < potential->maxrp; i++) {
      _zk[i] *= yk[i];
      a = fabs(_zk[i]); 
      if (a > max) max = a;
    }
    max1 = max*EPS5;
    max = max*EPS4;
    int maxrp = potential->maxrp;
    a = _zk[potential->maxrp-1];
    for (i = potential->maxrp-2; i >= 0; i--) {
      if (fabs(_zk[i] - a) > max1) {
	break;
      }
    }
    i1 = i;
    for (i = i1; i >= 0; i--) {      
      b = fabs(a - _zk[i]);
      _zk[i] = log(b);
      if (b > max) {
	break;
      }
    }
    i0 = i;
    if (i0 == i1) {
      i0--;
      b = fabs(a - _zk[i0]);
      _zk[i0] = log(b);
    }
    npts = i0+1;
    ic0 = npts;
    ic1 = npts+1;
    if (syk != NULL) {
      int size = sizeof(float)*(npts+2);
      syk->yk = malloc(size);
      AddMultiSize(yk_array, size);
      for (i = 0; i < npts ; i++) {
	syk->yk[i] = yk[i];
      }
      syk->yk[ic0] = a/pow(potential->rad[i0],k);
      n = i1 - i0 + 1;
      a = 0.0;
      b = 0.0;
      a2 = 0.0;
      b2 = 0.0;
      for (i = i0; i <= i1; i++) {      
	max = (potential->rad[i]-potential->rad[i0]);
	a += max;
	b += _zk[i];
	a2 += max*max;
	b2 += _zk[i]*max;
      }
      syk->yk[ic1] = (a*b - n*b2)/(a*a - n*a2);
      if (syk->yk[ic1] >= 0) {
	i1 = i0 + (i1-i0)*0.3;
	if (i1 == i0) i1 = i0 + 1;
	for (i = i0; i <= i1; i++) {      
	  max = (potential->rad[i]-potential->rad[i0]);
	  a += max;
	  b += _zk[i];
	  a2 += max*max;
	  b2 += _zk[i]*max;
	}
	syk->yk[ic1] = (a*b - n*b2)/(a*a - n*a2);
      }
      if (syk->yk[ic1] >= 0) {
	syk->yk[ic1] = -10.0/(potential->rad[i1]-potential->rad[i0]);
      }
#pragma omp flush
#pragma omp atomic write
      syk->npts = npts+2;
#pragma omp flush
    }
  }
  if (locked) ReleaseLock(lock);  
  if (yk_array->maxsize != 0) {
#pragma omp atomic
    yk_array->iset -= myrank;
  }
#pragma omp flush
  return 0;
}  

/* integrate a function given by f with two orbitals. */
/* type indicates the type of integral */
/* type = 1,    P1*P2 + Q1*Q2 */
/* type = 2,    P1*P2 */
/* type = 3,    Q1*Q2 */ 
/* type = 4:    P1*Q2 + Q1*P2 */
/* type = 5:    P1*Q2 - Q1*P2 */
/* type = 6:    P1*Q2 */
/* if type is positive, only the end point is returned, */
/* otherwise, the whole function is returned */
/* id indicate whether integrate inward (-1) or outward (0) */
int Integrate(double *f, ORBITAL *orb1, ORBITAL *orb2, 
	      int t, double *x, int id) {
  int i1, i2, ilast;
  int tm, i, j, k, type;
  double *r, a, dr;

  if (t == 0) t = 1;
  if (t < 0) {
    r = x;
    type = -t;
    tm = t;
  } else {
    r = _dwork;
    type = t;
    tm = -t;
  }
  for (i = 0; i < potential->maxrp; i++) {
    r[i] = 0.0;
  }

  ilast = Min(orb1->ilast, orb2->ilast);
  if (id >= 0) {
    IntegrateSubRegion(0, ilast, f, orb1, orb2, t, r, 0);
  } else {
    IntegrateSubRegion(0, ilast, f, orb1, orb2, t, r, -1);
  }
  i2 = ilast;
  if (orb1->ilast == ilast && orb1->n == 0) {
    i1 = ilast + 1;
    i2 = orb2->ilast;
    if (i2 > i1) {
      if (type == 6) {
	i = 7;
	if (t < 0) i = -i;
	IntegrateSubRegion(i1, i2, f, orb2, orb1, i, r, 1);
      } else {
	IntegrateSubRegion(i1, i2, f, orb1, orb2, t, r, 2);
      }
      i2--;
    }
    if (orb2->n == 0) {
      i1 = orb2->ilast + 1;
      i2 = potential->maxrp - 1;
      IntegrateSubRegion(i1, i2, f, orb1, orb2, tm, r, 3);
      i2--;
    }
  } else if (orb2->ilast == ilast && orb2->n == 0) {
    i1 = ilast + 1;
    i2 = orb1->ilast;
    if (i2 > i1) {
      IntegrateSubRegion(i1, i2, f, orb1, orb2, t, r, 1);
      i2--;
    }
    if (orb1->n == 0) {
      i1 = orb1->ilast + 1;
      i2 = potential->maxrp - 1;
      IntegrateSubRegion(i1, i2, f, orb1, orb2, tm, r, 3);
      i2--;
    }
  }
  if (orb1->n == 0 && orb2->n == 0) {
    if (orb1->energy != orb2->energy) {
      a = sqrt(2*orb1->energy*(1+0.5*FINE_STRUCTURE_CONST2*orb1->energy));
      dr = sqrt(2*orb2->energy*(1+0.5*FINE_STRUCTURE_CONST2*orb2->energy));
      dr = fabs(dr-a);
    } else {
      dr = sqrt(2*orb2->energy*(1+0.5*FINE_STRUCTURE_CONST2*orb2->energy));
    }
    dr = TWO_PI/dr;
    a = floor((0.1*potential->rad[i2])/dr);
    a = Max(1.0, a);
    a = Min(3.0, a);
    dr = potential->rad[i2]-dr*a;
    a = 0.0;
    k = Max(orb1->ilast, orb2->ilast);
    j = 0;
    for (i = i2; i > k; i--) {
      if (potential->rad[i] <= dr) break;
      a += r[i];
      j++;
    }
    if (j > 2) {
      a /= j;
      r[i2] = a;
    }
  }
  if (t >= 0) {
    *x = r[i2];
  } else {
    if (id >= 0) {
      for (i = i2+1; i < potential->maxrp; i++) {
	r[i] = r[i2];
      }
    } else {
      if (i2 > ilast) {
	for (i = ilast + 1; i <= i2; i++) {
	  r[i] = r[i2] - r[i];
	}
	for (i = i2+1; i < potential->maxrp; i++) {
	  r[i] = 0.0;
	}
	for (i = 0; i <= ilast; i++) {
	  r[i] = r[ilast+1] + r[i];
	}
      }
    }
  }
  
  return 0;
}

void AddEvenPoints(double *r, double *r1, int i0, int i1, int t) {
  int i;

  if (t < 0) {
    for (i = i0; i < i1; i++) {
      r[i] += r1[i];
    }
  } else {
    r[i1-1] += r1[i1-1];
  }
}

int IntegrateSubRegion(int i0, int i1, 
		       double *f, ORBITAL *orb1, ORBITAL *orb2,
		       int t, double *r, int m) {
  int i, j, ip, i2, type;
  ORBITAL *tmp;
  double *large1, *large2, *small1, *small2;
  double *x, *y, *r1, *x1, *x2, *y1, *y2;
  double a, b, e1, e2, a2, r0;
  int kv1 = orb1->kv;
  int kv2 = orb2->kv;
  double *vt1 = kv1<0?NULL:potential->VT[kv1];
  double *vt2 = kv2<0?NULL:potential->VT[kv2];
  
  if (i1 <= i0) return 0;
  type = abs(t);

  x = _dwork3;
  y = _dwork4;
  x1 = _dwork5;
  x2 = _dwork6;
  y1 = _dwork7;
  y2 = _dwork8;
  r1 = _dwork9;
  i2 = i1;

  switch (m) {
  case -1: /* m = -1 same as m = 0 but integrate inward */
  case 0:  /* m = 0 */
    large1 = Large(orb1);
    large2 = Large(orb2);
    small1 = Small(orb1);
    small2 = Small(orb2);
    switch (type) {
    case 1: /* type = 1 */
      for (i = i0; i <= i1; i++) {
	x[i] = large1[i] * large2[i];
	x[i] += small1[i] * small2[i];
	x[i] *= f[i]*potential->dr_drho[i];
      }
      if (i1 == orb1->ilast && orb1->n == 0 && i < potential->maxrp) {
	if (i <= orb2->ilast) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  x[i] = large1[i]*a*large2[i];
	  x[i] += (small1[i]*b + small1[ip]*a)*small2[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb2->n == 0) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  e1 = sin(large2[ip]);
	  e2 = cos(large2[ip]);
	  x[i] = large1[i]*a*large2[i]*e1;
	  x[i] += (small1[i]*b+small1[ip]*a)*(small2[i]*e2+small2[ip]*e1);
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      } else if (i1 == orb2->ilast && orb2->n == 0 && i < potential->maxrp) {
	if (i <= orb1->ilast) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  x[i] = large2[i]*a*large1[i];
	  x[i] += (small2[i]*b + small2[ip]*a)*small1[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb1->n == 0) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  e1 = sin(large1[ip]);
	  e2 = cos(large1[ip]);
	  x[i] = large2[i]*a*large1[i]*e1;
	  x[i] += (small2[i]*b+small2[ip]*a)*(small1[i]*e2+small1[ip]*e1);
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      }
      break;
    case 2: /* type = 2 */
      for (i = i0; i <= i1; i++) {
	x[i] = large1[i] * large2[i];
	x[i] *= f[i]*potential->dr_drho[i];
      }
      if (i1 == orb1->ilast && orb1->n == 0 && i < potential->maxrp) {
	if (i <= orb2->ilast) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  x[i] = large1[i]*a*large2[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb2->n == 0) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  e1 = sin(large2[ip]);
	  e2 = cos(large2[ip]);
	  x[i] = large1[i]*a*large2[i]*e1;
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      } else if (i1 == orb2->ilast && orb2->n == 0 && i < potential->maxrp) {
	if (i <= orb1->ilast) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  x[i] = large2[i]*a*large1[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb1->n == 0) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  e1 = sin(large1[ip]);
	  e2 = cos(large1[ip]);
	  x[i] = large2[i]*a*large1[i]*e1;
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      }
      break;
    case 3: /*type = 3 */
      for (i = i0; i <= i1; i++) {
	x[i] = small1[i] * small2[i];
	x[i] *= f[i]*potential->dr_drho[i];
      }
      if (i1 == orb1->ilast && orb1->n == 0 && i < potential->maxrp) {
	if (i <= orb2->ilast) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  x[i] = (small1[i]*b + small1[ip]*a)*small2[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb2->n == 0) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  e1 = sin(large2[ip]);
	  e2 = cos(large2[ip]);
	  x[i] = (small1[i]*b+small1[ip]*a)*(small2[i]*e2+small2[ip]*e1);
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      } else if (i1 == orb2->ilast && orb2->n == 0 && i < potential->maxrp) {
	if (i <= orb1->ilast) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  x[i] = (small2[i]*b + small2[ip]*a)*small1[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb1->n == 0) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  e1 = sin(large1[ip]);
	  e2 = cos(large1[ip]);
	  x[i] = (small2[i]*b+small2[ip]*a)*(small1[i]*e2+small1[ip]*e1);
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      }
      break;
    case 4: /*type = 4 */
      for (i = i0; i <= i1; i++) {
	x[i] = large1[i] * small2[i];
	x[i] += small1[i] * large2[i];
	x[i] *= f[i]*potential->dr_drho[i];
      }
      if (i1 == orb1->ilast && orb1->n == 0 && i < potential->maxrp) {
	if (i <= orb2->ilast) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  x[i] = large1[i]*a*small2[i];
	  x[i] += (small1[i]*b + small1[ip]*a)*large2[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb2->n == 0) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  e1 = sin(large2[ip]);
	  e2 = cos(large2[ip]);
	  x[i] = large1[i]*a*(small2[i]*e2+small2[ip]*e1);
	  x[i] += (small1[i]*b+small1[ip]*a)*large2[i]*e1;
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      } else if (i1 == orb2->ilast && orb2->n == 0 && i < potential->maxrp) {
	if (i <= orb1->ilast) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  x[i] = (small2[i]*b + small2[ip]*a)*large1[i];
	  x[i] += large2[i]*a*small1[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb1->n == 0) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  e1 = sin(large1[ip]);
	  e2 = cos(large1[ip]);
	  x[i] = (small2[i]*b+small2[ip]*a)*large1[i]*e1;
	  x[i] += large2[i]*a*(small1[i]*e2+small1[ip]*e1);
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      }
      break;
    case 5: /* type = 5 */
      for (i = i0; i <= i1; i++) {
	x[i] = large1[i] * small2[i];
	x[i] -= small1[i] * large2[i];
	x[i] *= f[i]*potential->dr_drho[i];
      } 
      if (i1 == orb1->ilast && orb1->n == 0 && i < potential->maxrp) {
	if (i <= orb2->ilast) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  x[i] = large1[i]*a*small2[i];
	  x[i] -= (small1[i]*b + small1[ip]*a)*large2[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb2->n == 0) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  e1 = sin(large2[ip]);
	  e2 = cos(large2[ip]);
	  x[i] = large1[i]*a*(small2[i]*e2+small2[ip]*e1);
	  x[i] -= (small1[i]*b+small1[ip]*a)*large2[i]*e1;
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      } else if (i1 == orb2->ilast && orb2->n == 0 && i < potential->maxrp) {
	if (i <= orb1->ilast) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  x[i] = (small2[i]*b + small2[ip]*a)*large1[i];
	  x[i] -= large2[i]*a*small1[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb1->n == 0) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  e1 = sin(large1[ip]);
	  e2 = cos(large1[ip]);
	  x[i] = (small2[i]*b+small2[ip]*a)*large1[i]*e1;
	  x[i] -= large2[i]*a*(small1[i]*e2+small1[ip]*e1);
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      }
      break;
    case 6: /* type = 6 */
      for (i = i0; i <= i1; i++) {
	x[i] = large1[i] * small2[i];
	x[i] *= f[i]*potential->dr_drho[i];
      }
      if (i1 == orb1->ilast && orb1->n == 0 && i < potential->maxrp) {
	if (i <= orb2->ilast) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  x[i] = large1[i]*a*small2[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb2->n == 0) {
	  ip = i+1;
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  e1 = sin(large2[ip]);
	  e2 = cos(large2[ip]);
	  x[i] = large1[i]*a*(small2[i]*e2+small2[ip]*e1);
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      } else if (i1 == orb2->ilast && orb2->n == 0 && i < potential->maxrp) {
	if (i <= orb1->ilast) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  x[i] = (small2[i]*b + small2[ip]*a)*large1[i];
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	} else if (orb1->n == 0) {
	  ip = i+1;
	  a = sin(large2[ip]);
	  b = cos(large2[ip]);
	  e1 = sin(large1[ip]);
	  e2 = cos(large1[ip]);
	  x[i] = (small2[i]*b+small2[ip]*a)*large1[i]*e1;
	  x[i] *= f[i]*potential->dr_drho[i];
	  i2 = i;
	}
      }
      break;
    default: /* error */
      return -1;
    }      
    NewtonCotes(r, x, i0, i2, t, m);
    break;

  case 1: /* m = 1 */
    if (type == 6) { /* type 6 needs special treatments */
      large1 = Large(orb1);
      large2 = Large(orb2);
      small1 = Small(orb1);
      small2 = Small(orb2);
      e1 = orb1->energy;
      e2 = orb2->energy;
      a2 = 0.5*FINE_STRUCTURE_CONST2;
      j = 0;
      for (i = i0; i <= i1; i+= 2) {
	ip = i+1;
	x[j] = large1[i] * small2[ip];
	x[j] *= f[i];
	y[j] = large1[i] * small2[i] * f[i];
	_phase[j] = large2[ip];
	_dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	j++;
      }
      if (i < potential->maxrp) {
	ip = i+1;
	if (i > orb1->ilast && orb1->n == 0) { 
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  b = small1[i]*b + small1[ip]*a;
	  a = large1[i]*a;
	  x[j] = a * small2[ip];
	  x[j] *= f[i];
	  y[j] = a * small2[i] * f[i];
	  _phase[j] = large2[ip];
	  _dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	  j++;
	  i2 = i;
	}
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      if (IsOdd(i2)) r[i2] = r[i2-1];
      break;
    } else if (type == 7) {
      large1 = Large(orb1);
      large2 = Large(orb2);
      small1 = Small(orb1);
      small2 = Small(orb2);
      e1 = orb1->energy;
      e2 = orb2->energy;
      a2 = 0.5*FINE_STRUCTURE_CONST2;
      j = 0;
      for (i = i0; i <= i1; i+= 2) {
	ip = i+1;
	x[j] = small1[i] * large2[i];
	x[j] *= f[i];
	_phase[j] = large2[ip];
	_dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	j++;	
      }
      if (i < potential->maxrp) {
	ip = i+1;
	if (i > orb1->ilast && orb1->n == 0) { 
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  b = small1[i]*b + small1[ip]*a;
	  x[j] = b * large2[i];
	  x[j] *= f[i];
	  _phase[j] = large2[ip];
	  _dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	  j++;
	  i2 = i;
	}
      }
      IntegrateSinCos(j, x, NULL, _phase, _dphase, i0, r, t);
      if (IsOdd(i2)) r[i2] = r[i2-1];
      break;
    }
  case 2: /* m = 1, 2 are essentially the same */
    /* type 6 is treated in m = 1 */
    if (m == 2) {
      tmp = orb1;
      orb1 = orb2;
      orb2 = tmp;
    }
    large1 = Large(orb1);
    large2 = Large(orb2);
    small1 = Small(orb1);
    small2 = Small(orb2);
    e1 = orb1->energy;
    e2 = orb2->energy;
    a2 = 0.5*FINE_STRUCTURE_CONST2;
    switch (type) {
    case 1: /* type = 1 */
      j = 0;
      for (i = i0; i <= i1; i+= 2) {
	ip = i+1;
	x[j] = large1[i] * large2[i];
	x[j] += small1[i] * small2[ip];
	x[j] *= f[i];
	y[j] = small1[i] * small2[i] * f[i];
	_phase[j] = large2[ip];
	_dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	j++;
      }
      if (i < potential->maxrp) {
	ip = i+1;
	if (i > orb1->ilast && orb1->n == 0) {
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  b = small1[i]*b + small1[ip]*a;
	  a = large1[i]*a;
	  x[j] = a * large2[i];
	  x[j] += b * small2[ip];
	  x[j] *= f[i];
	  y[j] = b * small2[i] * f[i];
	  _phase[j] = large2[ip];
	  _dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	  j++;
	  i2 = i;
	}
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      if (IsOdd(i2)) r[i2] = r[i2-1];
      break;
    case 2: /* type = 2 */
      j = 0;
      for (i = i0; i <= i1; i+= 2) {
	ip = i+1;
	x[j] = large1[i] * large2[i];
	x[j] *= f[i]; 
	_phase[j] = large2[ip];
	_dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	j++;
      }
      if (i < potential->maxrp) {
	ip = i+1;
	if (i > orb1->ilast && orb1->n == 0) {
	  a = sin(large1[ip]);
	  a = large1[i]*a;
	  x[j] = a * large2[i];
	  x[j] *= f[i];
	  _phase[j] = large2[ip];
	  _dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	  j++;
	  i2 = i;
	}
      }
      IntegrateSinCos(j, x, NULL, _phase, _dphase, i0, r, t);
      if (IsOdd(i2)) r[i2] = r[i2-1];
      break;
    case 3: /* type = 3 */
      j = 0;
      for (i = i0; i <= i1; i+= 2) {
	ip = i+1;
	x[j] = small1[i] * small2[ip];
	x[j] *= f[i];
	y[j] = small1[i] * small2[i] * f[i];
	_phase[j] = large2[ip];
	_dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	j++;
      }
      if (i < potential->maxrp) {
	ip = i+1;
	if (i > orb1->ilast && orb1->n == 0) {
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  b = small1[i]*b + small1[ip]*a;
	  x[j] += b * small2[ip];
	  x[j] *= f[i];
	  y[j] = b * small2[i] * f[i];
	  _phase[j] = large2[ip];
	  _dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	  j++;
	  i2 = i;
	}
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      if (IsOdd(i2)) r[i2] = r[i2-1];
      break;  
    case 4: /* type = 4 */
      j = 0;
      for (i = i0; i <= i1; i+= 2) {
	ip = i+1;
	x[j] = large1[i] * small2[ip];
	x[j] += small1[i] * large2[i];
	x[j] *= f[i];
	y[j] = large1[i] * small2[i] * f[i];
	_phase[j] = large2[ip];
	_dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	j++;
      }
      if (i < potential->maxrp) {
	ip = i+1;
	if (i > orb1->ilast && orb1->n == 0) {
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  b = small1[i]*b + small1[ip]*a;
	  a = large1[i]*a;
	  x[j] = a * small2[ip];
	  x[j] += b * large2[i];
	  x[j] *= f[i];
	  y[j] = a * small2[i] * f[i];
	  _phase[j] = large2[ip];
	  _dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	  j++;
	  i2 = i;
	}
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      if (IsOdd(i2)) r[i2] = r[i2-1];
      break;
    case 5: /* type = 5 */
      j = 0;
      if (m == 2) {
	r0 = r[i0];
	r[i0] = 0.0;
      }
      for (i = i0; i <= i1; i+= 2) {
	ip = i+1;
	x[j] = large1[i] * small2[ip];
	x[j] -= small1[i] * large2[i];
	x[j] *= f[i];
	y[j] = large1[i] * small2[i] * f[i];
	_phase[j] = large2[ip];
	_dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	j++;
      }
      if (i < potential->maxrp) {
	ip = i+1;
	if (i > orb1->ilast && orb1->n == 0) { 
	  a = sin(large1[ip]);
	  b = cos(large1[ip]);
	  b = small1[i]*b + small1[ip]*a;
	  a = large1[i]*a;
	  x[j] = a * small2[ip];
	  x[j] -= b * large2[i];
	  x[j] *= f[i];
	  y[j] = a * small2[i] * f[i];
	  _phase[j] = large2[ip];
	  _dphase[j] = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	  j++;
	  i2 = i;
	}
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      if (m == 2) {
	if (t < 0) {
	  for (i = i0; i <= i2; i++) {
	    r[i] = r0 - r[i];
	  }
	} else {
	  if (IsOdd(i2)) r[i2-1] = r0 - r[i2-1];
	  else r[i2] = r0 - r[i2];
	}
      }
      if (IsOdd(i2)) r[i2] = r[i2-1];
      break;
    default:
      return -1;
    }
    break;

  case 3: /* m = 3 */
    large1 = Large(orb1);
    large2 = Large(orb2);
    small1 = Small(orb1);
    small2 = Small(orb2);
    e1 = orb1->energy;
    e2 = orb2->energy;
    a2 = 0.5*FINE_STRUCTURE_CONST2;
    r1[i0] = 0.0;
    switch (type) {
    case 1: /* type = 1 */
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;	
	x1[j] = small1[i] * small2[ip];
	x2[j] = small1[ip] * small2[i];	
	x[j] = x1[j] + x2[j];
	x[j] *= 0.5*f[i];
	y1[j] = small1[i] * small2[i];
	y2[j] = small1[ip] * small2[ip];
	y2[j] += large1[i] * large2[i];
	y[j] = y1[j] - y2[j];
	y[j] *= 0.5*f[i];
	_phase[j] = large1[ip] + large2[ip];
	a = (1+a2*(e1-(vt1?vt1[i]:0.0)))/(large1[i]*large1[i]);
	b = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	_dphase[j] = a + b;
	_dphasep[j] = a - b;
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x[j] = -x1[j] + x2[j];
	x[j] *= 0.5*f[i];
	y[j] = y1[j] + y2[j];
	y[j] *= 0.5*f[i];
	_phase[j] = large1[ip] - large2[ip];
	j++;
      }
      IntegrateSinCos(j, NULL, y, _phase, _dphasep, i0, r1, t);
      AddEvenPoints(r, r1, i0, i1, t);
      break;	
    case 2: /* type = 2 */
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	y2[j] = -large1[i] * large2[i];
	y2[j] *= 0.5*f[i];
	y[j] = y2[j];
	_phase[j] = large1[ip] + large2[ip];
	a = (1+a2*(e1-(vt1?vt1[i]:0.0)))/(large1[i]*large1[i]);
	b = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	_dphase[j] = a + b;
	_dphasep[j] = a - b;
	j++;
      } 
      IntegrateSinCos(j, NULL, y, _phase, _dphase, i0, r, t);
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	y[j] = -y2[j];
	_phase[j] = large1[ip] - large2[ip];
	j++;
      }
      IntegrateSinCos(j, NULL, y, _phase, _dphasep, i0, r1, t);
      AddEvenPoints(r, r1, i0, i1, t);
      break;
    case 3: /* type = 3 */
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x1[j] = small1[i] * small2[ip];
	x2[j] = small1[ip] * small2[i];
	x[j] = x1[j] + x2[j];
	x[j] *= 0.5*f[i];
	y1[j] = small1[i] * small2[i];
	y2[j] = small1[ip] * small2[ip];
	y[j] = y1[j] - y2[j];
	y[j] *= 0.5*f[i];
	_phase[j] = large1[ip] + large2[ip];
	a = (1+a2*(e1-(vt1?vt1[i]:0.0)))/(large1[i]*large1[i]);
	b = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	_dphase[j] = a + b;
	_dphasep[j] = a - b;
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x[j] = -x1[j] + x2[j];
	x[j] *= 0.5*f[i];
	y[j] = y1[j] + y2[j];
	y[j] *= 0.5*f[i];
	_phase[j] = large1[ip] - large2[ip];
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphasep, i0, r1, t);
      AddEvenPoints(r, r1, i0, i1, t);
      break;
    case 4: /* type = 4 */
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x1[j] = large1[i] * small2[i];
	x2[j] = small1[i] * large2[i];
	x[j] = x1[j] + x2[j];
	x[j] *= 0.5*f[i];
	y[j] = -large1[i] * small2[ip];
	y[j] -= small1[ip] * large2[i];
	y[j] *= 0.5*f[i];
	y2[j] = y[j];
	_phase[j] = large1[ip] + large2[ip];	
	a = (1+a2*(e1-(vt1?vt1[i]:0.0)))/(large1[i]*large1[i]);
	b = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	_dphase[j] = a + b;
	_dphasep[j] = a - b;
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x[j] = x1[j] - x2[j];
	x[j] *= 0.5*f[i];
	y[j] = -y2[j];
	_phase[j] = large1[ip] - large2[ip];
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphasep, i0, r1, t);
      AddEvenPoints(r, r1, i0, i1, t);
      break;
    case 5: /* type = 5 */
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x1[j] = large1[i] * small2[i];
	x2[j] = small1[i] * large2[i];
	x[j] = x1[j] - x2[j];
	x[j] *= 0.5*f[i];
	y[j] = -large1[i] * small2[ip];
	y[j] += small1[ip] * large2[i];
	y[j] *= 0.5*f[i];
	y2[j] = y[j];
	_phase[j] = large1[ip] + large2[ip];
	a = (1+a2*(e1-(vt1?vt1[i]:0.0)))/(large1[i]*large1[i]);
	b = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	_dphase[j] = a + b;
	_dphasep[j] = a - b;
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x[j] = x1[j] + x2[j];
	x[j] *= 0.5*f[i];
	y[j] = -y2[j];
	_phase[j] = large1[ip] - large2[ip];
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphasep, i0, r1, t);
      AddEvenPoints(r, r1, i0, i1, t);
      break;
    case 6: /* type = 6 */
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x1[j] = large1[i] * small2[i];
	x1[j] *= 0.5*f[i];
	x[j] = x1[j];
	y[j] = -large1[i] * small2[ip];
	y[j] *= 0.5*f[i];
	y2[j] = y[j];
	_phase[j] = large1[ip] + large2[ip];
	a = (1+a2*(e1-(vt1?vt1[i]:0.0)))/(large1[i]*large1[i]);
	b = (1+a2*(e2-(vt2?vt2[i]:0.0)))/(large2[i]*large2[i]);
	_dphase[j] = a + b;
	_dphasep[j] = a - b;
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphase, i0, r, t);
      j = 0;
      for (i = i0; i <= i1; i += 2) {
	ip = i+1;
	x[j] = x1[j];
	y[j] = -y2[j];
	_phase[j] = large1[ip] - large2[ip];
	j++;
      }
      IntegrateSinCos(j, x, y, _phase, _dphasep, i0, r1, t);
      AddEvenPoints(r, r1, i0, i1, t);
      break;
    default:
      return -1;
    }
  default:
    return -1; 
  }

  return 0;
}

int IntegrateSinCos(int j, double *x, double *y, 
		    double *phase, double *dphase, 
		    int i0, double *r, int t) {
  int i, k, m, n, q, s, i1, qn;
  double si0, si1, cs0, cs1;
  double is0, is1, is2, is3;
  double ic0, ic1, ic2, ic3;
  double his0, his1, his2, his3;
  double hic0, hic1, hic2, hic3;
  double a0, a1, a2, a3;
  double d, p, h, dr, scx, scy;
  double *z, *u, *w, *u1, *w1, *xi, *yi, *wi;
  double mdp[2];
  
  w = _dwork14;
  z = _dwork10;
  u = _dwork11;
  xi = _dwork15;
  yi = _dwork16;
  wi = _dwork17;
  scx = 0.0;
  scy = 0.0;
  if (x) {
    for (i = 0; i < j; i++) {
      if (fabs(x[i]) > scx) scx = fabs(x[i]);
    }
    if (scx) {
      for (i = 0; i < j; i++) {
	x[i] /= scx;
      }
    }
  }
  if (y) {
    for (i = 0; i < j; i++) {
      if (fabs(y[i]) > scy) scy = fabs(y[i]);
    }
    if (scy) {
      for (i = 0; i < j; i++) {
	y[i] /= scy;
      }
    }
  }

  if (_intscm == 1) {
    return IntegrateSinCos1(j, x, scx, y, scy, phase, dphase, i0, r, t);
  }
  if (_intscm > 1) {
    return IntegrateSinCos2(j, x, scx, y, scy, phase, dphase, i0, r, t);
  }

  m = 0;
  mdp[0] = PI*_intscp1;
  mdp[1] = PI*_intscp2;
  d = fabs(phase[1]-phase[0]);
  if (d <= mdp[0]) q = 0;
  else if (d <= mdp[1]) q = 1;
  else q = 2;
  while (m < j-1) {
    s = m;
    if (q == 0) {
      for (i = m+1; i < j; i++) {
	d = fabs(phase[i]-phase[i-1]);
	if (d > mdp[0]) {
	  if (d > mdp[1]) qn = 2;
	  else qn = 1;
	  break;
	}
      }
    } else if (q == 1) {
      for (i = m+1; i < j; i++) {
	d = fabs(phase[i]-phase[i-1]);
	if (d <= mdp[0]) {
	  qn = 0;
	  break;
	} else if (d > mdp[1]) {
	  qn = 2;
	  break;
	}
      }
    } else {
      for (i = m+1; i < j; i++) {
	d = fabs(phase[i]-phase[i-1]);
	if (d <= mdp[1]) {
	  if (d <= mdp[0]) qn = 0;
	  else qn = 1;
	  break;
	}
      }
    }
    m = i-1;
    if (m >= j-3) m = j-1;
    if (q == 0) {
      for (i = s; i <= m; i++) {
	k = i0 + 2*i;
	z[k] = 0.0;
	if (x) z[k] += x[i]*sin(phase[i])*scx;
	if (y) z[k] += y[i]*cos(phase[i])*scy;
	z[k] *= potential->dr_drho[k];
	u[i] = potential->rho[k];
      }
      n = m-s+1;
      if (m < j-1) {
	i1 = n+1;
	u[n] = potential->rad[i0+2*(m+1)];	
      } else {
	i1 = n;
      }
      UVIP3C(3, i1, u+s, phase+s, wi+s, wi+j+s, u+j+s);
      if (x) {
	UVIP3C(3, i1, u+s, x+s, xi+s, xi+j+s, w+s);
      }
      if (y) {
	UVIP3C(3, i1, u+s, y+s, yi+s, yi+j+s, w+j+s);
      }
      for (i = s; i < m; i++) {
	k = i0 + 2*i + 1;
	z[k] = 0.0;
	a1 = wi[i];
	a2 = wi[i+j];
	a3 = u[i+j];
	a0 = phase[i] + a1 + a2 + a3;
	if (x) {
	  a1 = xi[i];
	  a2 = xi[i+j];
	  a3 = w[i];
	  z[k] += (x[i]+a1+a2+a3)*sin(a0)*scx;
	}
	if (y) {
	  a1 = yi[i];
	  a2 = yi[i+j];
	  a3 = w[i+j];
	  z[k] += (y[i]+a1+a2+a3)*cos(a0)*scy;
	}
	z[k] *= potential->dr_drho[k];
      }
      NewtonCotes(r, z, i0+2*s, i0+2*m, t, 0);
      //printf("q0: %d %d %d %d %d %12.5E %12.5E\n", j, s, m, i0+2*s, i0+2*m, r[i0+2*s], r[i0+2*m]);
    } else if (q == 1) {
      double *x1, *y1;
      int m1, s1;
      m1 = m+1;
      if (m1 >= j) m1 = j-1;
      s1 = s-1;
      if (s1 < 0) s1 = 0;
      if (x) x1 = x+s1;
      else x1 = NULL;
      if (y) y1 = y+s1;
      else y1 = NULL;
      k = i0+2*s1;
      if (s1 < s) {
	a0 = r[k];
	a1 = r[k+1];
	a2 = r[k+2];      
	r[k] = 0.0;
      }
      i1 = m1-s1+1;
      IntegrateSinCos2(i1, x1, scx, y1, scy, phase+s1, dphase+s1, k, r, t);
      if (s1 < s) {
	r[k] = a0;
	r[k+1] = a1;
	a2 -= r[k+2];
	for (i = s; i <= m; i++) {
	  r[i0+2*i] += a2;
	  r[i0+2*i+1] += a2;
	}
      }
      //printf("q1: %d %d %d %d %d %12.5E %12.5E\n", j, s, m, i0+2*s, i0+2*m, r[i0+2*s], r[i0+2*m]);
    } else {
      for (n = 1; n <= 5; n++) {
	i1 = s-n;
	if (i1 < 0 || dphase[i1]*dphase[s] <= 0) {
	  i1++;
	  break;
	}
      }
      if (t < 0) {
	for (i = i1; i <= m; i++) {
	  k = i0 + 2*i;
	  u[i] = potential->rad[k];
	  z[i] = potential->rad[k+1];
	}
	UVIP3P(3, m+1-i1, u+i1, phase+i1, m+1-s, z+s, w+s);
      }
      if (x) {
	for (i = i1; i <= m; i++) {
	  x[i] /= dphase[i];
	}
	UVIP3C(3, m+1-i1, phase+i1, x+i1, z+i1, z+j+i1, x+j+i1);
      }
      if (y) {
	for (i = i1; i <= m; i++) {
	  y[i] /= dphase[i];
	}
	UVIP3C(3, m+1-i1, phase+i1, y+i1, u+i1, u+j+i1, y+j+i1);
      }
      si0 = sin(phase[s]);
      cs0 = cos(phase[s]);
      for (i = s+1; i <= m; i++) {
	k = i0 + 2*i;
	if (t < 0) {
	  dr = w[i-1] - phase[i-1];
	  si1 = sin(w[i-1]);
	  cs1 = cos(w[i-1]);
	  his0 = -(cs1 - cs0);
	  hic0 = si1 - si0;
	  p = dr;
	  his1 = -p * cs1 + hic0;
	  hic1 = p * si1 - his0;
	  p *= dr; 
	  his2 = -p * cs1 + 2.0*hic1;
	  hic2 = p * si1 - 2.0*his1;
	  p *= dr;
	  his3 = -p * cs1 + 3.0*hic2;
	  hic3 = p * si1 - 3.0*his2;
	  r[k-1] = r[k-2];
	}
	d = phase[i] - phase[i-1];
	si1 = sin(phase[i]);
	cs1 = cos(phase[i]);
	is0 = -(cs1 - cs0);
	ic0 = si1 - si0;
	p = d;
	is1 = -p * cs1 + ic0;
	ic1 = p * si1 - is0;
	p *= d;
	is2 = -p * cs1 + 2.0*ic1;
	ic2 = p * si1 - 2.0*is1; 
	p *= d;
	is3 = -p * cs1 + 3.0*ic2;
	ic3 = p * si1 - 3.0*is2;
	r[k] = r[k-2];
	if (x != NULL) {
	  a0 = x[i-1];
	  a1 = z[i-1]; 
	  a2 = z[j+i-1];
	  a3 = x[j+i-1];
	  h = a0*is0 + a1*is1 + a2*is2 + a3*is3;
	  r[k] += h*scx;
	  h = a0*his0 + a1*his1 + a2*his2 + a3*his3;
	  r[k-1] += h*scx;      
	}
	if (y != NULL) {
	  a0 = y[i-1];
	  a1 = u[i-1];
	  a2 = u[j+i-1];
	  a3 = y[j+i-1];
	  h = a0*ic0 + a1*ic1 + a2*ic2 + a3*ic3;
	  r[k] += h*scy;
	  h = a0*hic0 + a1*hic1 + a2*hic2 + a3*hic3;
	  r[k-1] += h*scy;
	}
	si0 = si1;
	cs0 = cs1;
      }      
      //printf("q2: %d %d %d %d %d %12.5E %12.5E\n", j, s, m, i0+2*s, i0+2*m, r[i0+2*s], r[i0+2*m]);
    }
    q = qn;
  }
}

int IntegrateSinCos2(int j, double *x, double scx, double *y, double scy,
		     double *phase, double *dphase, 
		     int i0, double *r, int t) {
  int i, m, n, p, k, s, ip, iq, ip1, iq1, nj, ni;
  int *ix, *iy, mm, j0, j1;
  double d, dp, dr, a, xa, ya, a1, a2, a3;
  double *xi, *yi, *zi, *ri, *ui, *vi, *wi, *w;
  //static double t0, dt0 = 0, dt1 = 0.0, dt2 = 0.0, dt3 = 0.0;

  if (t > 0) t = -t;
  w = _dwork14;
  for (i = 0; i < j; i++) {
    w[i] = potential->rho[i0+i*2];
  }
  ri = _dwork10;
  wi = _dwork11;
  xi = _dwork15;
  yi = _dwork16;
  zi = _dwork17;
  vi = potential->W;
  ix = (int *) potential->dW;
  iy = (int *) potential->dW2;
  ui = potential->dW;

  mm = potential->maxrp-1;
  if (IsOdd(mm)) mm--;
  dp = PI*_intscp3;
  ip = 0;
  ni = 0;
  iy[0] = 0;
  while (ip < j-1) {
    //t0 = WallTime();
    n = 1;
    for (i = ip+1; i < j; i++) {
      d = fabs(phase[i]-phase[i-1]);
      m = (int)(1+d/dp);
      if (IsOdd(m)) m++;
      n += m;
      if (n > potential->maxrp) break;
    }
    iq = i-1;
    if (iq == ip) {
      iq = ip+1;
    }
    ri[0] = potential->rho[i0+2*ip];
    ix[0] = ip;
    k = 1;
    for (i = ip+1; i <= iq; i++) {
      d = fabs(phase[i]-phase[i-1]);
      m = (int)(1+d/dp);
      if (IsOdd(m)) m++;
      if (m > mm) m = mm;
      p = i0+2*i;      
      dr = (potential->rho[p]-potential->rho[p-2])/m;
      for (s = 0; s < m; s++,k++) {
	ri[k] = ri[k-1] + dr;
	ix[k] = i-1;
      }
      iy[i] = iy[i-1]+m;
    }
    //dt0 += WallTime()-t0;
    n = k;
    ip1 = ip-3;
    if (ip1 < 0) ip1 = 0;
    iq1 = iq+3;
    if (iq1 >= j) iq1 = j-1;
    nj = iq1-ip1+1;
    UVIP3C(3, nj, w+ip1, phase+ip1, wi+ip1, wi+j+ip1, w+j+ip1);
    if (x) {
      UVIP3C(3, nj, w+ip1, x+ip1, xi+ip1, xi+j+ip1, vi+ip1);
    }
    if (y) {
      UVIP3C(3, nj, w+ip1, y+ip1, yi+ip1, yi+j+ip1, vi+j+ip1);
    }
    //dt1 += WallTime()-t0;
    for (i = 0; i < n; i++) {
      zi[i] = 0.0;
      k = ix[i];
      a1 = wi[k];
      a2 = wi[j+k];
      a3 = w[j+k];
      d = ri[i]-w[k];
      a = phase[k] + d*(a1 + d*(a2 + d*a3));      
      if (x) {
	a1 = xi[k];
	a2 = xi[j+k];
	a3 = vi[k];
	xa = x[k] + d*(a1 + d*(a2 + d*a3));
	zi[i] += xa*sin(a)*scx;
      }
      if (y) {
	a1 = yi[k];
	a2 = yi[j+k];
	a3 = vi[j+k];
	ya = y[k] + d*(a1 + d*(a2 + d*a3));
	zi[i] += ya*cos(a)*scy;
      }
      p = i0+2*k;
      if (d < 1.) {
	dr = DrRho(potential, p, ri[i]);
      } else {
	dr = DrRho(potential, p+1, ri[i]);
      }
      zi[i] *= dr;
      /*
      printf("ip: %d %d %d %d %d %d %d %d %d %d %d %d %g %g %g %g %g %g %g %g\n",
	     j, i, k, i0,i0+2*k, iy[k], ip, ip1, iq, iq1, n, mm, d, ri[i],
	     a, xa, dr, zi[i], potential->dr_drho[p], potential->dr_drho[p+1]);
      */
    }
    //dt2 += WallTime()-t0;
    nj = iq-ip+1;
    k = i0+2*ip;
    for (i = ip+1; i <= iq; i++, k += 2) {
      j0 = iy[i-1]-iy[ip];
      j1 = iy[i]-iy[ip];
      ui[j0] = 0.0;
      NewtonCotes(ui, zi, j0, j1, t, 0);
      m = iy[i]-iy[i-1];
      d = (w[i]-w[i-1])/m;
      r[k+1] = r[k] + ui[j0+m/2]*d;
      r[k+2] = r[k] + ui[j0+m]*d;
    }          
    //dt3 += WallTime()-t0;
    ip = iq;
    ni++;    
  }
  //printf("dt: %10.3E %10.3E %10.3E %10.3E\n", dt0, dt1, dt2, dt3);
  return 0;
}

int IntegrateSinCos1(int j, double *x, double scx, double *y, double scy,
		     double *phase, double *dphase, 
		     int i0, double *r, int t) {
  int i, k, m, n, q, s, i1, nh;
  double si0, si1, cs0, cs1;
  double is0, is1, is2, is3;
  double ic0, ic1, ic2, ic3;
  double his0, his1, his2, his3;
  double hic0, hic1, hic2, hic3;
  double a0, a1, a2, a3;
  double d, p, h, dr;
  double *z, *u, *w, *u1, *w1;

  w = _dwork14;
  z = _dwork10;
  u = _dwork11;
  if (phase[j-1] < 0) {
    for (i = 0; i < j; i++) {
      phase[i] = -phase[i];
      dphase[i] = -dphase[i];
      if (x) x[i] = -x[i];
    }
  }
 
  nh = 0;
  for (i = 1, k = i0+2; i < j; i++, k += 2) {
    h = phase[i] - phase[i-1];
    z[k] = 0.0;
    if (x != NULL) z[k] += x[i]*sin(phase[i])*scx;
    if (y != NULL) z[k] += y[i]*cos(phase[i])*scy;
    z[k] *= potential->dr_drho[k];    
    if (i < 5) {
      if (h > 0.2) nh++;
      else nh = 0;
    } else {
      if (h > 0.1) nh++;
      else nh = 0;
    }    
    if (nh > 2) break;
  }
  if (i > 1) {
    z[i0] = 0.0;
    if (x != NULL) z[i0] += x[0]*sin(phase[0])*scx;
    if (y != NULL) z[i0] += y[0]*cos(phase[0])*scy;
    z[i0] *= potential->dr_drho[i0];
    if (i == j) {
      i1 = i;
    } else {
      i1 = i + 1;
    }
    u1 = u + i1;
    w1 = w + i1;
    for (m = 0, n = i0; m < i; m++, n += 2) {
      u[m] = potential->rad[n];
      w[m] = potential->rad[n+1];
      z[n+1] = 0.0;
    }
    if (i1 > i) {
      u[i] = potential->rad[n];
    }
    UVIP3P(3, i1, u, phase, i, w, w1);
    if (x) {
      UVIP3P(3, i1, u, x, i, w, u1);
      for (m = 0, n = i0+1; m < i; m++, n += 2) {
        z[n] += u1[m]*sin(w1[m])*scx;
      }
    }
    if (y) {
      UVIP3P(3, i1, u, y, i, w, u1);
      for (m = 0, n = i0+1; m < i; m++, n += 2) {
        z[n] += u1[m]*cos(w1[m])*scy;
      }
    }
    for (m = 0, n = i0+1; m < i; m++, n += 2) {
      z[n] *= potential->dr_drho[n];
    }
    NewtonCotes(r, z, i0, k-2, t, 0);
  }
  q = i-1;
  m = j-q;
  if (m < 2) {
    return 0;
  }

  for (n = 1; n <= 5; n++) {
    i1 = q-n;
    if (i1 < 0 || phase[i1] >= phase[i1+1]) {
      i1++;
      break;
    }
  }
  if (t < 0) {
    for (n = i1, s = i0; n < j; n++, s += 2) {
      u[n] = potential->rad[s];
      z[n] = potential->rad[s+1];
    }
    UVIP3P(3, j-i1, u+i1, phase+i1, m, z+q, w+q);
  }

  if (x != NULL) {
    for (n = i1; n < j; n++) {
      x[n] /= dphase[n];
    }
    UVIP3C(3, j-i1, phase+i1, x+i1, z+i1, z+j+i1, x+j+i1);
  }
  if (y != NULL) {
    for (n = i1; n < j; n++) {
      y[n] /= dphase[n];
    }
    UVIP3C(3, j-i1, phase+i1, y+i1, u+i1, u+j+i1, y+j+i1);
  }
  si0 = sin(phase[i-1]);
  cs0 = cos(phase[i-1]);
  for (; i < j; i++, k += 2) {
    if (t < 0) {
      dr = w[i-1] - phase[i-1];
      si1 = sin(w[i-1]);
      cs1 = cos(w[i-1]);
      his0 = -(cs1 - cs0);
      hic0 = si1 - si0;
      p = dr;
      his1 = -p * cs1 + hic0;
      hic1 = p * si1 - his0;
      p *= dr; 
      his2 = -p * cs1 + 2.0*hic1;
      hic2 = p * si1 - 2.0*his1;
      p *= dr;
      his3 = -p * cs1 + 3.0*hic2;
      hic3 = p * si1 - 3.0*his2;
      r[k-1] = r[k-2];
    }
    d = phase[i] - phase[i-1];
    si1 = sin(phase[i]);
    cs1 = cos(phase[i]);
    is0 = -(cs1 - cs0);
    ic0 = si1 - si0;
    p = d;
    is1 = -p * cs1 + ic0;
    ic1 = p * si1 - is0;
    p *= d;
    is2 = -p * cs1 + 2.0*ic1;
    ic2 = p * si1 - 2.0*is1; 
    p *= d;
    is3 = -p * cs1 + 3.0*ic2;
    ic3 = p * si1 - 3.0*is2;
    r[k] = r[k-2];
    if (x != NULL) {
      a0 = x[i-1];
      a1 = z[i-1]; 
      a2 = z[j+i-1];
      a3 = x[j+i-1];
      h = a0*is0 + a1*is1 + a2*is2 + a3*is3;
      r[k] += h*scx;
      h = a0*his0 + a1*his1 + a2*his2 + a3*his3;
      r[k-1] += h*scx;      
    }
    if (y != NULL) {
      a0 = y[i-1];
      a1 = u[i-1];
      a2 = u[j+i-1];
      a3 = y[j+i-1];
      h = a0*ic0 + a1*ic1 + a2*ic2 + a3*ic3;
      r[k] += h*scy;
      h = a0*hic0 + a1*hic1 + a2*hic2 + a3*hic3;
      r[k-1] += h*scy;
    }
    si0 = si1;
    cs0 = cs1;
  }

  return 0;
}

void LimitArrayRadial(int m, double n) {
  int i;
  
  if (m < 100) {
    n *= 1e6;
  }
  switch (m) {
  case -100:
    if (n < 0) n = ARYCTH;
    yk_array->cth = n;
    slater_array->cth = n;
    breit_array->cth = n;
    wbreit_array->cth = n;
    gos_array->cth = n;
    moments_array->cth = n;
    multipole_array->cth = n;
    residual_array->cth = n;
    vinti_array->cth = n;
    for (i = 0; i <= 4; i++) {
      xbreit_array[i]->cth = n;
    }
    break;
  case -1:
    LimitMultiSize(NULL, n);
    break;
  case 0:
    LimitMultiSize(yk_array, n);
    break;
  case 100:
    yk_array->cth = n;
    break;
  case 1:
    LimitMultiSize(slater_array, n);
    break;
  case 101:
    slater_array->cth = n;
    break;
  case 2:
    LimitMultiSize(breit_array, n);
    break;
  case 102:
    breit_array->cth = n;
    break;
  case 3:
    LimitMultiSize(wbreit_array, n);
    break;
  case 103:
    wbreit_array->cth = n;
    break;
  case 4:
    LimitMultiSize(gos_array, n);
    break;
  case 104:
    gos_array->cth = n;
    break;
  case 5:
    LimitMultiSize(moments_array, n);
    break;
  case 105:
    moments_array->cth = n;
    break;
  case 6:
    LimitMultiSize(multipole_array, n);
    break;
  case 106:
    multipole_array->cth = n;
    break;
  case 7:
    LimitMultiSize(residual_array, n);
    break;
  case 107:
    residual_array->cth = n;
    break;
  case 8:
    LimitMultiSize(vinti_array, n);
    break;
  case 108:
    vinti_array->cth = n;
    break;
  case 9:
    LimitMultiSize(qed1e_array, n);
    break;
  case 109:
    qed1e_array->cth = n;
    break;
  case 20:
  case 21:
  case 22:
  case 23:
  case 24:
    LimitMultiSize(xbreit_array[m-20], n);    
    break;
  case 120:
  case 121:
  case 122:
  case 123:
  case 124:
    xbreit_array[m-120]->cth = n;    
    break;
  default:
    printf("nothing is done\n");
    break;
  }
}

int InitRadial(void) {
  int ndim, i;
  int blocks[5] = {MULTI_BLOCK6,MULTI_BLOCK6,MULTI_BLOCK6,
		   MULTI_BLOCK6,MULTI_BLOCK6};
  potential = malloc(sizeof(POTENTIAL));
  hpotential = malloc(sizeof(POTENTIAL));
  rpotential = malloc(sizeof(POTENTIAL));
  potential->maxrp = 0;
  hpotential->maxrp = 0;
  rpotential->maxrp = 0;
  potential->nfrozen = 0;
  potential->npseudo = 0;
  potential->mpseudo = 0;
  potential->dpseudo = 1;
  potential->mode = POTMODE;
  potential->hxs = POTHXS;
  potential->ihx = POTIHX;
  potential->bhx = POTBHX;
  potential->chx = 1.0;
  potential->hx0 = POTHX0;
  potential->hx1 = POTHX1;
  potential->hlike = 0;
  potential->nse = qed.se;
  if (qed.mse >= 1000) {
    potential->hpvs = 1;
    qed.mse = qed.mse%1000;
  } else {
    potential->hpvs = 0;
  }
  potential->pse = qed.mse >= 140;
  qed.mse = qed.mse%100;
  potential->mse = qed.mse;
  potential->mvp = qed.vp%100;
  potential->pvp = qed.vp > 100;  
  potential->flag = 0;
  potential->rb = 0;
  potential->atom = GetAtomicNucleus();
  potential->ib = 0;
  potential->ib1 = 0;
  potential->ib0 = 0;
  potential->mps = -1;
  potential->kps = 0;
  potential->vxf = 0;
  potential->vxm = 0;
  potential->zps = 0;
  potential->nps = 0;
  potential->tps = 0;
  potential->rps = 0;
  potential->dps = 0;
  potential->aps = 0;
  potential->bps = 0;
  potential->nbt = -1.0;
  potential->nbs = 0.0;
  potential->nqf = 0.0;
  potential->efm = 0.0;
  potential->eth = 0.0;
  potential->fps = 0.0;
  potential->jps = 0.0;
  potential->gps = 0.0;
  potential->ips = 0;
  potential->sps = 0;
  potential->qps = 0;
  potential->sf0 = 0;
  potential->sf1 = 0;
  SetBoundaryMaster(0, 1.0, -1.0, 0.0);
  n_orbitals = 0;
  n_continua = 0;

  double cth = ARYCTH;
  orbitals = malloc(sizeof(ARRAY));
  if (!orbitals) return -1;
  if (ArrayInitID(orbitals, sizeof(ORBITAL), _orbitals_block, "orbitals") < 0) {
    return -1;
  }
  ndim = 5;
  slater_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(slater_array, sizeof(double), ndim, blocks, "slater_array");
  slater_array->cth = cth;
  
  ndim = 5;
  for (i = 0; i < ndim; i++) blocks[i] = MULTI_BLOCK5;
  breit_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(breit_array, sizeof(double *), ndim, blocks, "breit_array");
  breit_array->cth = cth;
  
  wbreit_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(wbreit_array, sizeof(double), ndim, blocks, "wbreit_array");
  wbreit_array->cth = cth;

  ndim = 3;
  for (i = 0; i < ndim; i++) blocks[i] = MULTI_BLOCK3;
  for (i = 0; i < 5; i++) {
    xbreit_array[i] = (MULTI *) malloc(sizeof(MULTI));
    char id[MULTI_IDLEN];
    sprintf(id, "xbreit_array%d", i);
    MultiInit(xbreit_array[i], sizeof(FLTARY), ndim, blocks, id);
    xbreit_array[i]->cth = cth;
  }
  
  ndim = 2;
  for (i = 0; i < ndim; i++) blocks[i] = MULTI_BLOCK2;
  residual_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(residual_array, sizeof(double), ndim, blocks, "residual_array");

  vinti_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(vinti_array, sizeof(double *), ndim, blocks, "vinti_array");

  qed1e_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(qed1e_array, sizeof(double), ndim, blocks, "qed1e_array");
  
  ndim = 3;
  for (i = 0; i < ndim; i++) blocks[i] = MULTI_BLOCK3;
  multipole_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(multipole_array, sizeof(double *), ndim, blocks, "multipole_array");
  multipole_array->cth = cth;

  ndim = 3;
  for (i = 0; i < ndim; i++) blocks[i] = MULTI_BLOCK3;
  moments_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(moments_array, sizeof(double), ndim, blocks, "moments_array"); 

  gos_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(gos_array, sizeof(double *), ndim, blocks, "gos_array");
  gos_array->cth = cth;

  yk_array = (MULTI *) malloc(sizeof(MULTI));
  MultiInit(yk_array, sizeof(FLTARY), ndim, blocks, "yk_array");
  yk_array->cth = cth;

  n_awgrid = 1;
  awgrid[0]= EPS3;
  
  SetRadialGrid(-1, -1.0, -1.0, -1.0, -1.0);
  SetSlaterCut(-1, -1);
  SetSlaterScale(-1, NULL, NULL, 1.0);

  SetOrbMap(0, 0, 0);

  PrepFermiNR();
  return 0;
}

void SetRadialCleanFlags(void) {  
  int i;
  SetMultiCleanFlag(slater_array);
  SetMultiCleanFlag(yk_array);
  SetMultiCleanFlag(breit_array);
  for (i = 0; i < 5; i++) {
    SetMultiCleanFlag(xbreit_array[i]);
  }
  SetMultiCleanFlag(wbreit_array);
  ReportMultiStats();
}

void SetSlaterScale(int m, char *s0, char *s1, double x) {
  int i0, i1, k0, k1, n0, n1, t;
  char sc0[128], sc1[128];
  SHELL *ss0, *ss1;
  double q0, q1;

  if (m < 0) {
    for (k0 = 0; k0 < MAXNSSC; k0++) {
      for (k1 = 0; k1 < MAXNSSC; k1++) {
	for (t = 0; t < MAXKSSC; t++) {
	  _slater_scale[t][k0][k1] = (float)x;
	}
      }
    }
    return;
  }
  strncpy(sc0, s0, 128);
  strncpy(sc1, s1, 128);
  n0 = ShellsFromString(sc0, &q0, &ss0);
  n1 = ShellsFromString(sc1, &q1, &ss1);
  for (i0 = 0; i0 < n0; i0++) {
    k0 = OrbitalIndex(ss0[i0].n, ss0[i0].kappa, 0);
    if (k0 >= MAXNSSC || k0 < 0) continue;
    for (i1 = 0; i1 < n1; i1++) {
      k1 = OrbitalIndex(ss1[i1].n, ss1[i1].kappa, 0);
      if (k1 >= MAXNSSC || k1 < 0) continue;
      _slater_scale[m][k0][k1] = (float)x;
      _slater_scale[m][k1][k0] = (float)x;
    }
  }
  if (n0) free(ss0);
  if (n1) free(ss1);
}

int ReinitRadial(int m) {
  if (m < 0) return 0;
#pragma omp barrier
#pragma omp master
  {
  SetSlaterScale(-1, NULL, NULL, 1.0);
  SetSlaterCut(-1, -1);
  ClearOrbitalTable(m);
  FreeSimpleArray(slater_array);
  FreeBreitArray();
  FreeSimpleArray(residual_array);
  FreeSimpleArray(qed1e_array);
  FreeMultipoleArray();
  FreeMomentsArray();
  FreeVintiArray();
  FreeYkArray();
  if (m < 2) {
    FreeGOSArray();
    int i;
    for (i = 0; i < potential->maxrp; i++) {
      potential->ZPS[i] = 0;
      potential->NPS[i] = 0;
      potential->EPS[i] = 0;
      potential->VPS[i] = 0;
    }
    if (m == 0) {      
      if (optimize_control.n_screen > 0) {
	free(optimize_control.screened_n);
	optimize_control.n_screen = 0;
      }
      SetAverageConfig(0, NULL, NULL, NULL);      
      potential->flag = 0;
      n_awgrid = 1;
      awgrid[0] = EPS3;
      SetRadialGrid(-1, -1.0, -1.0, -1.0, -1.0);
    }
  }
  }
#pragma omp barrier
  return 0;
}

int DensityAsymptote(double *d, double *a, double *b) {
  int i, m;
  double r, r2;
  
  for (i = 0; i < potential->maxrp; i++) {
    r = potential->rad[i];
    r2 = r*r;
    if (!d[i]) {
      break;
    }
    _zk[i] = log(d[i]/r2);
  }
  i -= 2;
  m = Min(10, i-2);
  
  double c[2];
  while (m < i) {
    PolyFit(2, c, i-m+1, potential->rad+m, _zk+m);
    r2 = 0;
    int j;
    for (j = m; j <= i; j++) {
      r = 1-exp(c[0]+c[1]*potential->rad[j]-_zk[j]);
      r *= r;
      if (r2 < r) r2 = r;
    }
    if (r2 < 1e-2) break;
    m++;
  }
  *a = exp(c[0]);
  *b = -c[1];
  //printf("dasym: %d %d %g %g\n", m, i, *a, *b);
  return m;
}

void ElectronDensity(char *ofn, int n, int *ilev, int t) {
  FILE *f;
  ANGULAR_ZMIX *ang;
  LEVEL *lev;
  ORBITAL *orb0, *orb1;
  int i, j, k, m, nz, iz;
  double a, *p0, *q0, *p1, *q1;

  f = fopen(ofn, "w");
  if (f == NULL) {
    printf("cannot open file %s\n", ofn);
    return;
  }

  for (i = 0; i < n; i++) {
    nz = AngularZMix(&ang, ilev[i], ilev[i], 0, 0, NULL, NULL);
    if (nz <= 0) {
      continue;
    }
    for (k = 0; k < potential->maxrp; k++) {
      _dwork15[k] = 0;
      for (iz = 0; iz < nz; iz++) {
	if (ang[iz].k != 0) continue;
	orb0 = GetOrbitalSolved(ang[iz].k0);
	orb1 = GetOrbitalSolved(ang[iz].k1);
	if (k > orb0->ilast || k > orb1->ilast) continue;
	j = GetJFromKappa(orb0->kappa);
	p0 = Large(orb0);
	q0 = Small(orb0);
	p1 = Large(orb1);
	q1 = Small(orb1);
	switch (t) {
	case 1:
	  a = p0[k]*p1[k] + q0[k]*q1[k];	  
	  break;
	case 2:
	  a = p0[k]*p1[k];
	  break;
	case 3:
	  a = q0[k]*q1[k];
	  break;
	case 4:
	  a = p0[k]*q1[k] + q0[k]*p1[k];
	  break;
	case 5:
	  a = p0[k]*q1[k] - q0[k]*p1[k];	  
	  break;
	case 6:
	  a = p0[k]*q1[k];
	  break;
	default:
	  a = 0.0;
	  break;
	}
	a *= ang[iz].coeff*sqrt(j+1.0);
	_dwork15[k] += a;
      }
    }
    if (nz > 0) free(ang);
    lev = GetLevel(ilev[i]);
    DecodePJ(lev->pj, NULL, &j);
    a = 1.0/sqrt(j+1.0);
    for (m = potential->maxrp-1; m >= 0; m--) {
      if (_dwork15[m]) break;
    }
    for (k = 0; k <= m; k++) {
      _dwork15[k] *= a;
    }
    Differential(_dwork15, _dwork14, 0, m, potential);
    double a0, a1;
    int ns = DensityAsymptote(_dwork15, &a0, &a1);
    fprintf(f, "# %4d %4d %4d %6d %6d %12.5E %12.5E\n",
	    i, n, m, t, ns, a0, a1);
    for (k = 0; k <= m; k++) {
      fprintf(f, "%6d %4d %15.8E %15.8E %15.8E\n",
	      i, k, potential->rad[k], _dwork15[k], _dwork14[k]);
    }
  }
  fclose(f);
}

void ExpectationValue(char *ifn, char *ofn, int n, int *ilev,
		      double a, int t0) {
  int ni, i, np, k, iz, nz, j, t;
  double *ri, *vi, r, v;
  FILE *f0, *f1;
  char buf[1024], *p;
  ORBITAL *orb0, *orb1;
  ANGULAR_ZMIX *ang;
  LEVEL *lev;

  t = abs(t0);
  f0 = fopen(ifn, "r");
  if (f0 == NULL) {
    printf("cannot open file %s\n", ifn);
    return;
  }
  ni = 0;
  while (1) {
    if (fgets(buf, 1024, f0) == NULL) break;
    p = buf;
    while(*p == ' ') p++;
    if (*p == '#') continue;
    k = sscanf(p, "%lg %lg", &r, &v);
    if (k != 2) continue;
    ni++;
  }  
  if (ni < 2) {
    printf("too few points in ExpectationValue input: %d\n", ni);
    fclose(f0);
    return;
  }
  ri = malloc(sizeof(double)*ni);
  vi = malloc(sizeof(double)*ni);
  fseek(f0, 0, SEEK_SET);
  i = 0;
  while (1) {
    if (fgets(buf, 1024, f0) == NULL) break;
    p = buf;
    while(*p == ' ') p++;
    if (*p == '#') continue;
    k = sscanf(buf, "%lg %lg", &r, &v);
    if (k != 2) continue;
    ri[i] = potential->ar*pow(r, potential->qr) + potential->br*log(r);
    vi[i] = v;
    i++;
  }
  fclose(f0);
  np = 3;
  for (i = 0; i < potential->maxrp; i++) {
    _dwork14[i] = potential->ar*pow(potential->rad[i], potential->qr) +
      potential->br*log(potential->rad[i]);
  }
  UVIP3P(np, ni, ri, vi, potential->maxrp, _dwork14, _dwork15);
  if (t0 < 0) {
    for (i = 0; i < potential->maxrp; i++) {
      _dwork15[i] = exp(_dwork15[i]);
    }
  }
  if (a) {
    for (i = 0; i < potential->maxrp; i++) {
      _dwork15[i] *= pow(potential->rad[i], a);
    }
  }
  f1 = fopen(ofn, "w");
  if (f1 == NULL) {
    printf("cannot open file: %s\n", ofn);
    free(ri);
    free(vi);
    return;
  }
  fprintf(f1, "# %4d %g %d\n", n, a, t);
  for (i = 0; i < n; i++) {
    nz = AngularZMix(&ang, ilev[i], ilev[i], 0, 0, NULL, NULL);
    if (nz <= 0) {
      continue;
    }
    v = 0;
    for (iz = 0; iz < nz; iz++) {
      if (ang[iz].k != 0) continue;
      orb0 = GetOrbitalSolved(ang[iz].k0);
      orb1 = GetOrbitalSolved(ang[iz].k1);
      Integrate(_dwork15, orb0, orb1, t, &r, 0);
      j = GetJFromKappa(orb0->kappa);
      v += r * ang[iz].coeff * sqrt(j+1.0);
    }
    lev = GetLevel(ilev[i]);
    DecodePJ(lev->pj, NULL, &j);
    v /= sqrt(j+1.0);
    fprintf(f1, "%6d %20.12E\n", ilev[i], v);
    free(ang);
  }  
  fclose(f1);  
}

int TestIntegrate(void) {
  int n, m;
  double r, r0;  

  m = -2;
  for (n = 0; n < potential->maxrp; n++) {
    _xk[n] = pow(potential->rad[n], m);
  }

  double e = 9e3/HARTREE_EV;
  int k = OrbitalIndex(0, -1, e);
  ORBITAL *orb = GetOrbital(k);
  WaveFuncTable("ws.txt", 0, -1, e);
  Integrate(_xk, orb, orb, -1, _zk, 0);
  for (n = 0; n < potential->maxrp; n++) {
    printf("%d %g %g %g\n", n, potential->rad[n], _xk[n], _zk[n]);
  }

  return 0;
}

void RemoveOrbitalLock(void) {
  if (orbitals->lock) {
    DestroyLock(orbitals->lock);
    free(orbitals->lock);
    orbitals->lock = NULL;
  }
}

int TestIntegrate0(void) {
  ORBITAL *orb1, *orb2, *orb3, *orb4;
  int k1, k2, k3, k4, k, i, i0=1500;
  double r, a, s[6];
 
  k1 = OrbitalIndex(2, -2, 0);
  k2 = OrbitalIndex(3, -1, 0);
  k3 = OrbitalIndex(0, -5, 3.30265398e3/HARTREE_EV);
  k4 = OrbitalIndex(0, -4, 2.577e3/HARTREE_EV);
  printf("# %d %d %d %d\n", k1, k2, k3, k4);
  orb1 = GetOrbital(k1);
  orb2 = GetOrbital(k2);
  orb3 = GetOrbital(k3);
  orb4 = GetOrbital(k4);

  for (k = 1; k < 7; k++) {    
    for (i = 0; i < potential->maxrp; i++) {
      _yk[i] = pow(potential->rad[i],k);
      _yk[i+i0] = 1.0/_yk[i];
    }
    Integrate(_yk+i0, orb3, orb4, -1, _zk, 0);
    Integrate(_yk+i0, orb3, orb4, -1, _zk+i0, -1);
    for (i = 0; i < potential->maxrp; i++) {
      printf("%d %4d %15.8E %15.8E %15.8E %15.8E %15.8E\n", 
	     k, i, potential->rad[i], _yk[i], _zk[i], _yk[i+i0], _zk[i+i0]);
    }
    Integrate(_yk+i0, orb3, orb4, 1, &r, 0);
    Integrate(_yk+i0, orb3, orb4, 1, &a, -1);
    printf("# %15.8E %15.8E\n", r, a);
    printf("\n\n");
  }
  
  return 0;
}

void OptimizeModSE(int n, int ka, double dr, int ni) {
  double e0, eps, e, r0, r1, r, z, rs, rms;
  ORBITAL *orb;
  int k, i;

  z = potential->atom->atomic_number;
  if (z < 60) eps = 1e-4;
  else eps = 1e-5;
  r = potential->atom->rmse;
  rms = potential->atom->rms*1e5*RBOHR;
  if (r <= 0) {
    r = rms;
  }
  dr = r*dr;
  k = OrbitalIndex(n, ka, 0);
  orb = GetOrbitalSolved(k);
  e0 = HydrogenicSelfEnergy(31, 0, 1.0, potential, orb, orb);
  if (potential->N > 1) {
    if (orb->rorb == NULL) {
      orb->rorb = SolveAltOrbital(orb, rpotential);
    }
    if (orb->horb == NULL) {
      orb->horb = SolveAltOrbital(orb, hpotential);
    }
    rs = SelfEnergyRatio(orb->rorb, orb->horb);
    e0 *= rs;
  } else {
    rs = 1.0;
  }
  INIQED(z, 9, 1, r);
  e = HydrogenicSelfEnergy(51, 0, 1.0, potential, orb, orb);
  if (ni > 0 && fabs(e0-e) > eps) {
    i = 0;
    if (e < e0) {
      r0 = r;
      r1 = r0;
      while (i < ni && e < e0) {
	r1 = r1 + dr;
	INIQED(z, 9, 1, r1);
	e = HydrogenicSelfEnergy(51, 0, 1.0, potential, orb, orb);
	if (fabs(e0-e) < eps) {
	  r = r1;
	  break;
	}
	i++;
      }
    } else {
      r1 = r;
      r0 = r1;
      while (i < ni && e > e0) {
	r0 = r0 - dr;
	INIQED(z, 9, 1, r0);
	e = HydrogenicSelfEnergy(51, 0, 1.0, potential, orb, orb);
	if (fabs(e0-e) < eps) {
	  r = r0;
	  break;
	}
	i++;
      }
    }
    i = 0;
    while(i < ni) {
      r = 0.5*(r0 + r1);
      INIQED(z, 9, 1, r);
      e = HydrogenicSelfEnergy(51, 0, 1.0, potential, orb, orb);
      if (fabs(e-e0) < eps) break;
      if (r1-r0 < 1e-5) break;
      if (e < e0) {
	r0 = r;
      } else if (e > e0) {
	r1 = r;
      }
      i++;
    }
  }
  
  printf("modqed rms: %3.0f %18.10E %18.10E %18.10E %18.10E %18.10E %18.10E\n",
	 z, rms, r, rs, e, e0, e-e0);
}

int AddNewConfigToList(int k, int ni, int *kc,
		       CONFIG *c0, int nb, int **kcb,
		       int nc, SHELL_RESTRICTION *sr,
		       int checknew, int mar) {
  CONFIG *cfg = ConfigFromIList(ni, kc);
  int r;
  double sth;
  if (nc > 0) {
    r = ApplyRestriction(1, cfg, nc, sr);
    if (r <= 0) {
      FreeConfigData(cfg);
      free(cfg);
      return -1;
    }
  }
  if (mar != 3 && checknew && ConfigExists(cfg)) {
    FreeConfigData(cfg);
    free(cfg);
    return -1;
  }
  sth = c0->sth;
  if (sth > 0 || mar >= 3) {
    int i0, i1, i2, i3;
    int n0, k0, n1, k1, n2, k2, n3, k3;
    int i, j;
    double s = 0;
    int sms0 = qed.sms;
    int br0 = qed.br;
    qed.sms = 0;
    qed.br = 0;
    for (i = 0; i < nb; i++) {
      i0 = -1;
      i1 = -1;
      i2 = -1;
      i3 = -1;
      for (j = 0; j < ni; j++) {
	if (kc[j] == kcb[i][j]+2) {
	  if (i1 >= 0 || i3 >= 0) break;	  
	  i1 = j;
	  i3 = j;
	} else if (kc[j] == kcb[i][j]-2) {
	  if (i0 >= 0 || i2 >= 0) break;
	  i0 = j;
	  i2 = j;
	} else if (kc[j] == kcb[i][j]+1) {
	  if (i1 < 0) i1 = j;
	  else if (i3 < 0) i3 = j;
	  else break;
	} else if (kc[j] == kcb[i][j]-1) {
	  if (i0 < 0) i0 = j;
	  else if (i2 < 0) i2 = j;
	  else break;
	} else if (abs(kc[j]-kcb[i][j]) > 2) {
	  break;
	}
      }
      if (j < ni) continue;
      if (i0 < 0 || i1 < 0) continue;
      IntToShell(i0, &n0, &k0);
      IntToShell(i1, &n1, &k1);
      int i2s0, i2s1, i2s;
      int i3s0, i3s1, i3s;
      if (i2 >= 0) {
	IntToShell(i2, &n2, &k2);
	i2s0 = 0;
	i2s1 = 1;
      } else {
	i2s0 = 0;
	i2s1 = cfg->n_shells;
	//n2 = cfg->shells[0].n;
	//k2 = cfg->shells[0].kappa;
      }
      if (i3 >= 0) {
	i3s0 = 0;
	i3s1 = 1;
	IntToShell(i3, &n3, &k3);
      } else {
	i3s0 = 0;
	i3s1 = cfg->n_shells;
	//n3 = cfg->shells[0].n;
	//k3 = cfg->shells[0].kappa;
      }
      int ko0, ko1, ko2, ko3;
      ORBITAL *o0, *o1, *o2, *o3;
      int j0, kl0, j1, kl1, j2, kl2, j3, kl3;
      ko0 = OrbitalIndex(n0, k0, 0);
      QED1E(ko0, ko0);
      ko1 = OrbitalIndex(n1, k1, 0);
      QED1E(ko1, ko1);
      GetJLFromKappa(k0, &j0, &kl0);
      GetJLFromKappa(k1, &j1, &kl1);
      o0 = GetOrbital(ko0);
      o1 = GetOrbital(ko1);
      for (i2s = i2s0; i2s < i2s1; i2s++) {
	if (i2 < 0) {
	  n2 = cfg->shells[i2s].n;
	  k2 = cfg->shells[i2s].kappa;
	  if (n2 == n0 && k2 == k0 && cfg->shells[i2s].nq < 2) continue;
	}
	ko2 = OrbitalIndex(n2, k2, 0);
	QED1E(ko2, ko2);
	GetJLFromKappa(k2, &j2, &kl2);
	o2 = GetOrbital(ko2);
	for (i3s = i3s0; i3s < i3s1; i3s++) {
	  if (i3 < 0) {
	    if (i2 < 0 && i2s != i3s) continue;
	    n3 = cfg->shells[i3s].n;
	    k3 = cfg->shells[i3s].kappa;
	    if (n3 == n1 && k3 == k1 && cfg->shells[i3s].nq < 2) continue;
	  }
	  ko3 = OrbitalIndex(n3, k3, 0);
	  QED1E(ko3, ko3);
	  GetJLFromKappa(k3, &j3, &kl3);
	  if (IsOdd((kl0+kl1+kl2+kl3)/2)) continue;
	  o3 = GetOrbital(ko3);
	  //double de = fabs(o1->energy-o0->energy + o3->energy-o2->energy);
	  double de = fabs(o1->energy+o1->qed-o0->energy-o0->qed + o3->energy+o3->qed-o2->energy-o2->qed);
	  if (mar == 4) {
	    if (c0->mde > de) c0->mde = de;
	    continue;
	  }
	  if (mar < 3 && de < EPS10) {
	    s = 1e10;
	    break;
	  } else if (mar < 3 && c0->cth > 0) {
	    s = c0->cth/de;
	  } else {
	    double s2 = 0, s1 = 0, sd = 0, se = 0;
	    int kk, ks[4];
	    ks[0] = ko0;
	    ks[1] = ko2;
	    ks[2] = ko1;
	    ks[3] = ko3;
	    double s2b = 0;
	    for (kk = 0; kk < 5; kk += 2) {
	      SlaterTotal(&sd, &se, NULL, ks, kk, 0);
	      s2b += (sd*sd+se*se)/(kk+1);
	    }
	    double s1b = 0;
	    if (i2 < 0 && i3 < 0 && k0 == k1) {
	      ResidualPotential(&s1b, ko0, ko1);
	      s1b = s1b*s1b;
	    }
	    s2 = sqrt(s2b+s1b);
	    if (mar < 3) {
	      s2 /= de;
	    }
	    if (s < s2) s = s2;
	  }
	}
      }
    }  
    qed.sms = sms0;
    qed.br = br0;
    if (mar < 3) {
      if (s < sth) {
	FreeConfigData(cfg);
	free(cfg);
	return -10;
      }
    } else if (mar == 3) {
      c0->cth = Max(c0->cth, s);
    }
  }
  r = -1;
  if (mar < 3) {
    if (Couple(cfg) < 0) {
      FreeConfigData(cfg);
      free(cfg);
      return -1;
    }
    r = AddConfigToList(k, cfg);
  }
  if (r < 0) {
    FreeConfigData(cfg);
  }
  free(cfg);
  return r;
}

int ConfigSD(int m0r, int ng, int *kg, char *s, char *gn1, char *gn2,
	     int n0, int n1, int n0d, int n1d, int k0, int k1,
	     int ngb, int *kgb, double sth) {
  int ni, nr, *kc, nb, **kcb, i, j, k, ir, ns, ks, ks2, ka, is, js;
  int t, ird, nd, kd, kd2, jd, id, ig1, ig2, m0, tnc;
  CONFIG_GROUP *g;
  CONFIG *c, *cr;
  SHELL_RESTRICTION *sr;
  int m, mar, *kcr, nc, *kcrn, nnr, nn, km, kt, km0;
  double sth0;

  sr = NULL;
  nc = 0;
  if (s) {
    nc = GetRestriction(s, &sr, 0);
  }
  m0 = abs(m0r);
  if (m0 == 0) {
    ig1 = GroupIndex(gn1);
    if (ig1 < 0) {
      printf("invalid config group name: %s\n", gn1);
      if (nc > 0) {
	for (i = 0; i < nc; i++) {
	  free(sr[i].shells);
	}
	free(sr);
      }
      return -1;
    }
    nr = GetConfigFromString(&cr, s);
    if (nr <= 0) {
      if (nc > 0) {
	for (i = 0; i < nc; i++) {
	  free(sr[i].shells);
	}
	free(sr);
      }
      g = GetGroup(ig1);
      if (g != NULL && g->n_cfgs == 0) RemoveGroup(ig1);
      return 0;
    }
    ni = 0;
    for (i = 0; i < nr; i++) {
      cr[i].sth = sth;
      if (ni < cr[i].shells[0].n) ni = cr[i].shells[0].n;
    }
    for (i = 0; i < ng; i++) {
      g = GetGroup(kg[i]);
      if (ni < g->nmax) ni = g->nmax;
    }
    ni = ni*ni;
    kc = malloc(sizeof(int)*ni);
    nb = 0;
    if (ngb > 0) {
      if (kgb) {
	for (i = 0; i < ngb; i++) {
	  g = GetGroup(kgb[i]);
	  nb += g->n_cfgs;
	}
	kcb = NULL;
	if (nb > 0) {
	  kcb = malloc(sizeof(int *)*nb);
	  for (i = 0; i < nb; i++) {
	    kcb[i] = malloc(sizeof(int)*ni);
	  }
	}
	k = 0;
	for (i = 0; i < ngb; i++) {
	  g = GetGroup(kgb[i]);
	  for (j = 0; j < g->n_cfgs; j++, k++) {
	    c = GetConfigFromGroup(kgb[i], j);
	    ConfigToIList(c, ni, kcb[k]);
	  }
	}
      } 
    }
    for (i = 0; i < nr; i++) {
      ConfigToIList(&cr[i], ni, kc);
      AddNewConfigToList(ig1, ni, kc, &cr[i], nb, kcb, nc, sr, 1, 0);
      cr[i].sth = 0;
    }
    
    free(kc);
    if (nb > 0) {
      for (i = 0; i < nb; i++) {
	free(kcb[i]);
      }
      free(kcb);
    }
    if (nc > 0) {
      for (i = 0; i < nc; i++) {
	free(sr[i].shells);
      }
      free(sr);
    }
    if (sth > 0) ReinitRadial(2);
    g = GetGroup(ig1);
    if (g != NULL && g->n_cfgs == 0) RemoveGroup(ig1);
    return 0;
  }
  
  if (n0d <= 0) n0d = n0;
  if (n1d <= 0) n1d = n1;
  if (k0 < 0) k0 = 0;
  if (k1 < 0) k1 = Max(n1,n1d)-1;
  if (gn2 == NULL || strlen(gn2) == 0) gn2 = gn1;
  m = m0%10;
  mar = m0/10;
  if (m < 1 || m > 3) {
    printf("invalid mode: %d %d %d\n", m0, m, mar);
    return -1;
  }
  if (mar == 9) {
    if (ng > 0 && kg) {
      for (i = 0; i < ng; i++) {
	g = GetGroup(kg[i]);
	for (j = 0; j < g->n_cfgs; j++) {
	  c = GetConfigFromGroup(kg[i], j);
	  c->sth = sth;
	}
      }
    }
    return 0;
  }
  ni = Max(n1, n1d);
  tnc = 0;
  for (i = 0; i < ng; i++) {
    g = GetGroup(kg[i]);
    tnc += g->n_cfgs;
    if (ni < g->nmax) ni = g->nmax;
  }
  if (kgb && kgb != kg) {
    for (i = 0; i < ngb; i++) {
      g = GetGroup(kgb[i]);
      if (ni < g->nmax) ni = g->nmax;
    }
  }
  ni = ni*ni;
  int fcr = 0;
  if (mar > 0) sth = 0;
  if (mar == 2) {
    nr = 1;
    cr = NULL;
  } else {
    if (s == NULL || strlen(s) == 0) {
      nr = tnc;
      cr = malloc(sizeof(CONFIG)*tnc);
      t = 0;
      for (i = 0; i < ng; i++) {
	g = GetGroup(kg[i]);
	for (j = 0; j < g->n_cfgs; j++) {
	  c = GetConfigFromGroup(kg[i], j);
	  memcpy(&cr[t], c, sizeof(CONFIG));
	  t++;
	}
      }	
    } else {
      nr = GetConfigFromString(&cr, s);
      fcr = 1;
    }
  }
  if (mar == 1) {
    n0 = 1;
    n1 = 1;
    k0 = 0;
    k1 = 0;
    n0d = 1;
    n1d = 1;    
  }
  if (nr <= 0) {
    printf("invalid reference shell spec in ConfigSD: %s\n", s);
    if (nc > 0) {
      for (i = 0; i < nc; i++) {
	free(sr[i].shells);
      }
      free(sr);
    }
    return 0;
  }
  kc = malloc(sizeof(int)*ni);
  kcr = NULL;  
  if (cr) {
    for (i = 0; i < ni; i++) kc[i] = 0;
    for (i = 0; i < nr; i++) {
      for (j = 0; j < cr[i].n_shells; j++) {
	k = ShellToInt(cr[i].shells[j].n, cr[i].shells[j].kappa);
	if (k >= 0 && k < ni) kc[k] = 1;
      }
      if (fcr) free(cr[i].shells);
    }  
    free(cr);
    nr = 0;
    for (i = 0; i < ni; i++) {
      if (kc[i]) nr++;
    }
    kcr = malloc(sizeof(int)*nr);
    j = 0;
    for (i = 0; i < ni; i++) {
      if (kc[i]) {
	kcr[j] = i;
	j++;
      }
    }
  }
  if (mar < 3) {
    ig1 = GroupIndex(gn1);
    if (ig1 < 0) {
      printf("invalid config group name: %s\n", gn1);
      return -1;
    }
    ig2 = GroupIndex(gn2);
    if (ig2 < 0) {
      printf("invalid config group name: %s\n", gn2);
      return -1;
    }
  } else {
    ig1 = -1;
    ig2 = -1;
  }
  nb = 0;
  if (ngb > 0) {
    if (mar >= 3) kgb = NULL;
    if (kgb && sth >= 0) {
      for (i = 0; i < ngb; i++) {
	g = GetGroup(kgb[i]);
	nb += g->n_cfgs;
      }
      if (nb > 0) {
	kcb = malloc(sizeof(int *)*nb);
	for (i = 0; i < nb; i++) {
	  kcb[i] = malloc(sizeof(int)*ni);
	}
      }
      k = 0;
      for (i = 0; i < ngb; i++) {
	g = GetGroup(kgb[i]);
	for (j = 0; j < g->n_cfgs; j++, k++) {
	  c = GetConfigFromGroup(kgb[i], j);
	  ConfigToIList(c, ni, kcb[k]);
	}
      }
    } else {
      nb = 1;
      kcb = malloc(sizeof(int *)*nb);
      kcb[0] = malloc(sizeof(int)*ni);
    }
  }
  nnr = nr;
  kcrn = malloc(sizeof(int)*(nnr+1));
  if (kcr) {
    km0 = -1;
    nnr = 0;
    for (k = 0; k < nr; k++) {
      ir = kcr[k];
      IntToShell(ir, &nn, &km);
      km = GetLFromKappa(km);
      if (km != km0) {
	kcrn[nnr] = k;
	km0 = km;
	nnr++;
      }
    }
    kcrn[nnr] = nr;
  } else {
    kcrn[0] = 0;
    kcrn[1] = 1;
  }
  if (ng > 0 && kg) {
    for (i = 0; i < ng; i++) {
      g = GetGroup(kg[i]);
      for (j = 0; j < g->n_cfgs; j++) {
	c = GetConfigFromGroup(kg[i], j);
	c->sth = sth;
      }
    }
    if (sth < -EPS10) {
      if (ngb <= 0 || !kgb) {
	printf("sth < 0 without kgb: %g %d %d\n", sth, ngb, ng);
	return -1;
      }
      GetInteractConfigs(ngb, kgb, ng, kg, -sth);
    }
  }
  if (m != 2) {
    for (i = 0; i < ng; i++) {
      g = GetGroup(kg[i]);
      for (j = 0; j < g->n_cfgs; j++) {
	c = GetConfigFromGroup(kg[i], j);
	if (c->sth < -EPS10) continue;
	ConfigToIList(c, ni, kc);
	if ((sth < 0 || kgb == NULL) && nb == 1) {
	  memcpy(kcb[0], kc, sizeof(int)*ni);
	}
	for (km = 0; km < nnr; km++) {	
	  for (ns = n0; ns <= n1; ns++) {
	    for (ks = k0; ks <= k1; ks++) {
	      if (ks >= ns) break;
	      int pr = 1000000;
	      for (k = kcrn[km]; k < kcrn[km+1]; k++) {
		if (kcr) {
		  ir = kcr[k];
		  if (kc[ir] <= 0) continue;
		  if (mar == 3) {
		    int ntmp, ktmp;
		    IntToShell(ir, &ntmp, &ktmp);
		    if (ntmp < c->shells[0].n) continue;
		  }
		} else {
		  ir = -1;
		}
		ks2 = 2*ks;
		for (js = ks2-1; js <= ks2+1; js += 2) {
		  if (js < 0) continue;
		  if (mar != 1) {
		    ka = GetKappaFromJL(js, ks2);
		    is = ShellToInt(ns, ka);
		    if (kc[is] == js+1) continue;
		  } else {
		    is = -1;
		  }
		  if (ir >= 0 && ir == is) continue;
		  if (ir >= 0) kc[ir]--;
		  if (is >= 0) kc[is]++;
		  if (pr == 1000000 || mar >= 3) {
		    pr = AddNewConfigToList(ig1, ni, kc, c, nb, kcb,
					    nc, sr, m0r > 0, mar);
		  } else if (pr > -10) {
		    double sth0 = c->sth;
		    c->sth = 0.0;
		    AddNewConfigToList(ig1, ni, kc, c, nb, kcb,
				       nc, sr, m0r > 0, mar);
		    c->sth = sth0;
		  }
		  if (ir >= 0) kc[ir]++;
		  if (is >= 0) kc[is]--;
		}
	      }
	    }
	  }
	}
      }
    }
  }
  if (m != 1) {
    for (i = 0; i < ng; i++) {
      g = GetGroup(kg[i]);
      for (j = 0; j < g->n_cfgs; j++) {
	c = GetConfigFromGroup(kg[i], j);
	if (c->sth < -EPS10) continue;
	ConfigToIList(c, ni, kc);
	if ((sth < 0 || kgb == NULL) && nb == 1) {
	  memcpy(kcb[0], kc, sizeof(int)*ni);
	}
	for (km = 0; km < nnr; km++) {	
	  for (ns = n0; ns <= n1; ns++) {
	    for (kt = 0; kt < nnr; kt++) {
	      for (nd = ns; nd <= n1d; nd++) {
		if (nd < n0d) continue;
		for (ks = k0; ks <= k1; ks++) {
		  if (ks >= ns) break;
		  for (kd = k0; kd <= k1; kd++) {
		    if (kd >= nd) break;
		    int pr = 1000000;
		    ks2 = 2*ks;
		    for (js = ks2-1; js <= ks2+1; js += 2) {
		      if (js < 0) continue;
		      if (mar != 1) {
			ka = GetKappaFromJL(js, ks2);
			is = ShellToInt(ns, ka);
			if (kc[is] == js+1) continue;
		      } else {
			is = -1;
		      }	
		      kd2 = 2*kd;
		      for (jd = kd2-1; jd <= kd2+1; jd += 2) {
			if (jd < 0) continue;	  
			for (k = kcrn[km]; k < kcrn[km+1]; k++) {
			  if (kcr) {
			    ir = kcr[k];
			    if (kc[ir] <= 0) continue;
			    if (mar == 3) {
			      int ntmp, ktmp;
			      IntToShell(ir, &ntmp, &ktmp);
			      if (ntmp < c->shells[0].n) continue;
			    }
			  } else {
			    ir = -1;
			  }
			  for (t = kcrn[kt]; t < kcrn[kt+1]; t++) {
			    if (kcr) {
			      ird = kcr[t];
			      if (kc[ird] <= 0) continue;
			      if (mar == 3) {
				int ntmp, ktmp;
				IntToShell(ird, &ntmp, &ktmp);
				if (c->shells[0].nq > 1) {
				  if (ntmp < c->shells[0].n) continue;
				} else if (c->n_shells > 1) {
				  if (ntmp < c->shells[1].n) continue;
				}
			      }
			    } else {
			      ird = -1;
			    }
			    if (mar != 1) {
			      ka = GetKappaFromJL(jd, kd2);
			      id = ShellToInt(nd, ka);
			      if (kc[id] == jd+1) continue;
			    } else {
			      id = -1;
			    }
			    if (ir >= 0 && is == ir &&
				ird >= 0 && id == ird) continue;
			    if (ir >= 0) kc[ir]--;			      
			    if (is >= 0) kc[is]++;
			    if (ird >= 0) kc[ird]--;
			    if (id >= 0) kc[id]++;
			    if ((ir < 0 || kc[ir] >= 0) &&
				(ird < 0 || kc[ird] >= 0) &&
				(is < 0 || kc[is] <= js+1) &&
				(id < 0 || kc[id] <= jd+1)) {
			      if (pr == 1000000 || mar >= 3) {
				pr = AddNewConfigToList(ig2, ni, kc, c,
							nb, kcb, nc, sr,
							m0r>0, mar);
			      } else if (pr > -10) {
				double sth0 = c->sth;
				c->sth = 0.0;
				AddNewConfigToList(ig2, ni, kc, c,
						   nb, kcb, nc, sr,
						   m0r>0, mar);
				c->sth = sth0;
			      }
			    }
			    if (ird >= 0) kc[ird]++;
			    if (id >= 0) kc[id]--;
			    if (ir >= 0) kc[ir]++;
			    if (is >= 0) kc[is]--;
			  }
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
  free(kc);
  if (kcr) free(kcr);
  free(kcrn);
  if (nc > 0) {
    for (i = 0; i < nc; i++) {
      free(sr[i].shells);
    }
    free(sr);
  }
  if (nb > 0) {
    for (i = 0; i < nb; i++) {
      free(kcb[i]);
    }
    free(kcb);
  }
  if (mar < 3) {
    if (sth > 0) ReinitRadial(2);
    g = GetGroup(ig2);
    if (g != NULL && g->n_cfgs == 0) RemoveGroup(ig2);
    if (ig1 != ig2) {
      g = GetGroup(ig1);
      if (g != NULL && g->n_cfgs == 0) RemoveGroup(ig1);
    }
    if (ng > 0 && kg) {
      for (i = 0; i < ng; i++) {
	g = GetGroup(kg[i]);
	for (j = 0; j < g->n_cfgs; j++) {
	  c = GetConfigFromGroup(kg[i], j);
	  c->sth = 0;
	}
      }
    }
  } else {
    ReinitRadial(2);
  }
  return 0;
}

void PlasmaScreen(int m, int vxf,
		  double zps, double nps, double tps, double ups) {
  potential->mps = m%10;
  potential->kps = m/10;
  potential->vxf = vxf;
  potential->zps = 0;
  potential->nps = 0;
  potential->tps = 0;
  potential->rps = 0;
  if (m < 0 || nps <= 0) return;
  potential->zps = zps;
  potential->nps = nps*pow(RBOHR,3);
  potential->tps = tps/HARTREE_EV;
  //the old SP mode is handled by mps=0 now
  if (potential->mps == 2) potential->mps = 0;
  //stewart&pyatt model, ups is the z*;
  if (ups >= 0) potential->ups = ups;
  if (zps > 0) {
    if (m == 0) {
      potential->rps = pow(3*zps/(FOUR_PI*potential->nps),ONETHIRD);
    } else {
      potential->rps = sqrt(potential->tps/(FOUR_PI*potential->nps*zps));
    }
  }
}

void SetOrbNMax(int kmin, int kmax, int nmax) {
  int ie = OnErrorOrb();
  SetOnErrorOrb(-1);
  if (kmax < 0) {
    int n = SetNMaxKappa(kmin, nmax);
    SetOnErrorOrb(ie);
    return;
  }
  ResetWidMPI();
#pragma omp parallel default(shared)
  {
    int kappa, k, k2, j;
    for (k = kmin; k <= kmax; k++) {
      k2 = 2*k;
      for (j = -1; j <= 1; j += 2) {
	kappa = GetKappaFromJL(k2+j, k2);
	if (kappa == 0) continue;
	if (SkipMPI()) continue;
	int n = SetNMaxKappa(kappa, nmax);
	int idx = -1;
	double e = 0.0;
	if (n > k && n < 1000000000) {
	  idx = OrbitalIndex(n, kappa, 0);
	  e = 0.0;
	  if (idx >= 0) {
	    ORBITAL *orb = GetOrbitalSolved(idx);
	    e = orb->energy;
	  }
	}
	if (_orbnmax_print) {
	  MPrintf(-1, "nmax = %3d %3d %3d %3d %3d %12.5E\n",
		  k, j, kappa, n, idx, e);
	}
      }
    }
  }
  SetOnErrorOrb(ie);
}

int GetOrbNMax(int kappa, int j) {
  int k;
  if (j != 0) {
    kappa = GetKappaFromJL(kappa*2+j, kappa*2);
    if (kappa == 0) return -1;
  }
  k = ((abs(kappa)-1)*2) + (kappa>0);
  if (k >= _korbmap) {
    printf("too large kappa in GetOrbNMax: %d %d %d\n", kappa, k, _korbmap);
    return -1;
  }
  return _orbmap[k].nmax;
}
    
int SetNMaxKappa(int kappa, int nmax) {
  int k = ((abs(kappa)-1)*2) + (kappa>0);
  if (k >= _korbmap) {
    printf("too large kappa in SetNMaxKappa: %d %d %d\n", kappa, k, _korbmap);
    return 0;
  }
  ORBMAP *om = &_orbmap[k];
  if (potential->mps < 0) {
    om->nmax = 1000000000;
    return om->nmax;
  }
  ORBITAL orb;
  int m = GetLFromKappa(kappa)/2;
  int n = m+1;
  int i = 0;
  while (1) {
    orb.n = n;
    orb.kappa = kappa;
    orb.energy = 0;
    i = RadialSolver(&orb, potential); 
    if (i < 0) break;
    if (orb.energy >= 0) {
      i = -1;
      break;
    }
    if (n >= nmax) break;
    n *= 2;
  }
  if (i == 0) {
    om->nmax = -n;
    return om->nmax;
  }
  int n0 = n/2;
  int n1 = n;
  while (n1-n0>1) {
    n = (n0+n1)/2;
    orb.n = n;
    orb.kappa = kappa;
    orb.energy = 0;
    i = RadialSolver(&orb, potential);
    if (i < 0 || orb.energy >= 0) {
      n1 = n;
    } else {
      n0 = n;
    }
  }
  om->nmax = n0;
  return om->nmax;
}

void SetOptionRadial(char *s, char *sp, int ip, double dp) {
  if (0 == strcmp(s, "radial:refine_maxfun")) {
    _refine_maxfun = ip;
    return;
  }
  if (0 == strcmp(s, "radial:refine_msglvl")) {
    _refine_msglvl = ip;
    return;
  }
  if (0 == strcmp(s, "radial:orbitals_block")) {
    _orbitals_block = ip;
    return;
  }
  if (0 == strcmp(s, "radial:refine_np")) {
    _refine_np = ip;
    return;
  }
  if (0 == strcmp(s, "radial:refine_xtol")) {
    _refine_xtol = dp;
    if (_refine_xtol <= 0) _refine_xtol = EPS5;
    return;
  }
  if (0 == strcmp(s, "radial:refine_scale")) {
    _refine_scale = dp;
    if (_refine_scale <= 0) _refine_scale = 0.1;
    return;
  }
  if (0 == strcmp(s, "radial:refine_pj")) {
    _refine_pj = ip;
    return;
  }
  if (0 == strcmp(s, "radial:refine_em")) {
    _refine_em = ip;
    return;
  }
  if (0 == strcmp(s, "radial:acfg_wmode")) {
    _acfg_wmode = ip;
    return;
  }
  if (0 == strcmp(s, "radial:maxnhd")) {
    InitHydrogenicDipole(ip);
    return;
  }
  if (0 == strcmp(s, "radial:awmin")) {
    _awmin = dp;
    return;
  }
  if (0 == strcmp(s, "radial:config_energy")) {
    _config_energy = ip;
    return;
  }
  if (0 == strcmp(s, "radial:config_energy_psr")) {
    _config_energy_psr = ip;
    return;
  }
  if (0 == strcmp(s, "radial:config_energy_pfn")) {
    strncpy(_config_energy_pfn, sp, 1023);
    return;
  }
  if (0 == strcmp(s, "radial:config_energy_csi")) {
    _config_energy_csi = dp;
    return;
  }
  if (0 == strcmp(s, "radial:sturm_idx")) {
    _sturm_idx = dp;
    return;
  }
  if (0 == strcmp(s, "radial:sturm_eref")) {
    _sturm_eref = dp/HARTREE_EV;
    return;
  }
  if (0 == strcmp(s, "radial:sturm_nref")) {
    _sturm_nref = ip;
    return;
  }
  if (0 == strcmp(s, "radial:sturm_kref")) {
    _sturm_kref = ip;
    return;
  }
  if (0 == strcmp(s, "radial:bhx")) {
    potential->bhx = dp;
    return;
  }
  if (0 == strcmp(s, "radial:chx")) {
    potential->chx = dp;
    return;
  }
  if (0 == strcmp(s, "radial:hxs")) {
    potential->hxs = dp;
    return;
  }
  if (0 == strcmp(s, "radial:hx0")) {
    potential->hx0 = dp;
    return;
  }
  if (0 == strcmp(s, "radial:hx1")) {
    potential->hx1 = dp;
    return;
  }
  if (0 == strcmp(s, "radial:ihx")) {
    potential->ihx = dp;
    return;
  }
  if (0 == strcmp(s, "radial:mhx")) {
    _mhx = ip;
    return;
  }
  if (0 == strcmp(s, "radial:xhx")) {
    _xhx = dp;
    return;
  }
  if (0 == strcmp(s, "radial:ihxmin")) {
    _ihxmin = dp;
    return;
  }
  if (0 == strcmp(s, "radial:print_spm")) {
    _print_spm = ip;
    return;
  }
  if (0 == strcmp(s, "radial:slater_kmax")) {
    _slater_kmax = ip;
    return;
  }
  if (0 == strcmp(s, "radial:orbnmax_print")) {
    _orbnmax_print = ip;
    return;
  }
  if (0 == strcmp(s, "radial:psemax")) {
    _psemax = dp;
    return;
  }
  if (0 == strcmp(s, "radial:psnfmin")) {
    _psnfmin = ip;
    return;
  }
  if (0 == strcmp(s, "radial:psnfmax")) {
    _psnfmax = ip;
    return;
  }
  if (0 == strcmp(s, "radial:psnmin")) {
    _psnmin = ip;
    return;
  }
  if (0 == strcmp(s, "radial:psnmax")) {
    _psnmax = ip;
    return;
  }
  if (0 == strcmp(s, "radial:pskmax")) {
    _pskmax = ip;
    return;
  }
  if (0 == strcmp(s, "radial:sc_print")) {
    _sc_print = ip;
    return;
  }
  if (0 == strcmp(s, "radial:sc_niter")) {
    _sc_niter = ip;
    return;
  }
  if (0 == strcmp(s, "radial:sc_wmin")) {
    _sc_wmin = dp;
    return;
  }
  if (0 == strcmp(s, "radial:hx_wmin")) {
    _hx_wmin = dp;
    return;
  }
  if (0 == strcmp(s, "radial:rfsta")) {
    _rfsta = dp;
    return;
  }
  if (0 == strcmp(s, "radial:dfsta")) {
    _dfsta = dp;
    return;
  }
  if (0 == strcmp(s, "radial:maxsta")) {
    _maxsta = dp;
    return;
  }
  if (0 == strcmp(s, "radial:minsta")) {
    _minsta = dp;
    return;
  }
  if (0 == strcmp(s, "radial:minke")) {
    _minke = dp;
    return;
  }
  if (0 == strcmp(s, "radial:incsta")) {
    _incsta = dp;
    return;
  }
  if (0 == strcmp(s, "radial:sc_vxf")) {
    _sc_vxf = ip;
    return;
  }
  if (0 == strcmp(s, "radial:vxm")) {
    potential->vxm = ip;
    return;
  }
  if (0 == strcmp(s, "radial:znbaa")) {
    _znbaa = ip;
    return;
  }
  if (0 == strcmp(s, "radial:sc_lepton")) {
    _sc_lepton = ip;
    return;
  }
  if (0 == strcmp(s, "radial:sc_maxiter")) {
    _sc_maxiter = ip;
    return;
  }
  if (0 == strcmp(s, "radial:sc_mass")) {
    _sc_mass = dp;
    return;
  }
  if (0 == strcmp(s, "radial:sc_tol")) {
    _sc_tol = dp;
    return;
  }
  if (0 == strcmp(s, "radial:sc_fmrp")) {
    _sc_fmrp = dp;
    return;
  }
  if (0 == strcmp(s, "radial:sc_charge")) {
    _sc_charge = dp;
    return;
  }
  if (0 == strcmp(s, "radial:sc_cfg")) {
    strncpy(_sc_cfg, sp, 1023);
    return;
  }
  if (0 == strcmp(s, "radial:print_maxiter")) {
    _print_maxiter = ip;
    return;
  }
  if (0 == strcmp(s, "radial:free_threshold")) {
    _free_threshold = dp;
    return;
  }
  if (0 == strcmp(s, "radial:csi")) {
    _csi = dp;
    return;
  }
  if (0 == strcmp(s, "radial:wsi")) {
    _wsi = dp;
    return;
  }
  if (0 == strcmp(s, "radial:gridasymp")) {
    _gridasymp = dp;
    potential->asymp = _gridasymp;
    return;
  }
  if (0 == strcmp(s, "radial:gridratio")) {
    _gridratio = dp;
    potential->ratio = _gridratio;
    return;
  }
  if (0 == strcmp(s, "radial:gridrmin")) {
    _gridrmin = dp;
    potential->rmin = _gridrmin;
    return;
  }
  if (0 == strcmp(s, "radial:gridrmax")) {
    _gridrmax = dp;
    return;
  }
  if (0 == strcmp(s, "radial:gridemax")) {
    _gridemax = dp/HARTREE_EV;
    return;
  }
  if (0 == strcmp(s, "radial:gridqr")) {
    _gridqr = dp;
    potential->qr = _gridqr;
    return;
  }
  if (0 == strcmp(s, "radial:maxrp")) {
    _maxrp = ip;
    AllocWorkSpace(_maxrp);
    return;
  }
  if (0 == strcmp(s, "radial:intscm")) {
    _intscm = ip;
    return;
  }
  if (0 == strcmp(s, "radial:intscp1")) {
    _intscp1 = dp;
    return;
  }
  if (0 == strcmp(s, "radial:intscp2")) {
    _intscp2 = dp;
    return;
  }
  if (0 == strcmp(s, "radial:intscp3")) {
    _intscp3 = dp;
    return;
  }
}
