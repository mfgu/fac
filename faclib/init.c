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

static char *rcsid="$Id$";
#if __GNUC__ == 2
#define USE(var) static void * use_##var = (&use_##var, (void *) &var) 
USE (rcsid);
#endif

#include "init.h"
#include "mpiutil.h"

#if FAC_DEBUG
  FILE *debug_log = NULL;
#endif

#ifdef PERFORM_STATISTICS
  FILE *perform_log = NULL;
#endif

double HARTREE_EV;
double RYDBERG_EV;
double RATE_AU;
double RATE_AU10;
double RATE_AU12;
double AREA_AU20;
double VOLUME_AU;
double RBOHR;
double VAU8;
double AMU;
double FINE_STRUCTURE_CONST;
double FINE_STRUCTURE_CONST2;

int Info(void) {
  printf("========================================\n");
  printf("The Flexible Atomic Code (FAC)\n");
  printf("Version %d.%d.%d\n", VERSION, SUBVERSION, SUBSUBVERSION);
  printf("Bugs and suggestions, please contact:\n");
  printf("Ming Feng Gu, mfgu@ssl.berkeley.edu\n");
  printf("========================================\n");
  return 0;
}

void SetLepton(int t, double m0, double e0, char *fn) {
  FILE *f;
  double m, e;
  
  switch(t) {
  case 0: //electron
    m = 1.0;
    e = 1.0;
    break;
  case 1: //muon
    m = 2.06768283e2;
    e = 1.0;
    break;
  case 2: //tau
    m = 3.47722828e3;
    e = 1;
    break;
  default:
    m = 1.0;
    e = 1.0;
    break;
  }
  if (m0 > 0) m = m0;
  if (e0 > 0) e = e0;

  double e2 = e*e;
  double e3 = e2*e;
  double e4 = e2*e2;
  double e6 = e3*e3;
  
  HARTREE_EV = _HARTREE_EV*(m*e4);
  RYDBERG_EV = _RYDBERG_EV*(m*e4);
  RATE_AU = _RATE_AU*(m*e4);
  RATE_AU10 = _RATE_AU10*(m*e4);
  RATE_AU12 = _RATE_AU12*(m*e4);
  AREA_AU20 = _AREA_AU20*(m*e4);
  VOLUME_AU = _VOLUME_AU/(m*m*m*e6);
  RBOHR = _RBOHR/(m*e2);
  VAU8 = _VAU8*e2;
  AMU = _AMU/m;
  FINE_STRUCTURE_CONST = _FINE_STRUCTURE_CONST*e2;
  FINE_STRUCTURE_CONST2 = _FINE_STRUCTURE_CONST2*e4;

  if (fn) {
    if (strcmp(fn, "-") == 0) {
      f = stdout;
    } else {
      f = fopen(fn, "w");
    }
    if (f == NULL) {
      printf("cannot open file %s\n", fn);
      return;
    }
    fprintf(f, "m=%g\n", m);
    fprintf(f, "e=%g\n", e);
    fprintf(f, "HARTREE_EV=%g\n", HARTREE_EV);
    fprintf(f, "RATE_AU=%g\n", RATE_AU);
    fprintf(f, "RBOHR=%g\n", RBOHR);
    fprintf(f, "AMU=%g\n", AMU);
    fprintf(f, "FSC=%g\n", FINE_STRUCTURE_CONST);
    if (f != stdout) fclose(f);
  }
}
  
int InitFac(void) {
  int ierr;
  
#if FAC_DEBUG
  debug_log = fopen("debug.log", "w");
#endif

#ifdef PERFORM_STATISTICS
  perform_log = fopen("perform.log", "w");
#endif

  SetLepton(0, 0, 0, NULL);
  //InitializeMPI(0);
  ierr = InitConfig();
  if ( ierr < 0) {
    printf("initialize failed in InitConfig\n");
    return ierr;
  }

  InitMultiStats();
  InitNucleus();
  InitCoulomb();
  InitAngular();
  InitRecouple();

  ierr = InitRadial();
  if (ierr < 0) {
    printf("initialize failed in InitRadial\n");
    return ierr;
  }
  
  InitDBase();
  InitStructure();
  InitExcitation();
  InitRecombination();
  InitIonization();
  InitRMatrix();
  InitMBPT();

  return 0;
}

int ReinitFac(int m_config, int m_recouple, int m_radial,
	      int m_dbase, int m_structure, int m_excitation,
	      int m_recombination, int m_ionization) {
  ReinitExcitation(m_excitation);
  ReinitRecombination(m_recombination);
  ReinitIonization(m_ionization);
  ReinitRecouple(m_recouple);
  ReinitRadial(m_radial);
  ReinitDBase(m_dbase);
  ReinitStructure(m_structure);
  ReinitConfig(m_config);

  return 0;
}

void SetOption(char *s, char *sp, int ip, double dp) {
  if (strstr(s, "dbase:") == s) {
    SetOptionDBase(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "mbpt:") == s) {
    SetOptionMBPT(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "radial:") == s) {
    SetOptionRadial(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "orbital:") == s) {
    SetOptionOrbital(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "structure:") == s) {
    SetOptionStructure(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "transition:") == s) {
    SetOptionTransition(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "excitation:") == s) {
    SetOptionExcitation(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "rmatrix:") == s) {
    SetOptionRMatrix(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "ionization:") == s) {
    SetOptionIonization(s, sp, ip, dp);
    return;
  }
  if (strstr(s, "recombination:") == s) {
    SetOptionRecombination(s, sp, ip, dp);
    return;
  }
  return;
}
