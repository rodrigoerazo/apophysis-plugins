/*
    Apophysis Plugin

    Copyright (C) 2007-2009 Joel Faber
    Copyright (C) 2007-2009 Michael Faber

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

typedef struct
{
	double pre_log_base;
	double denom;
} Variables;

#define LOG_BASE_DEF 2.71828182845905

//#define APO_VIRTUALVAR

#include "plugin.h"

APO_PLUGIN("pre_log");
APO_VARIABLES(
	VAR_REAL(pre_log_base, LOG_BASE_DEF)
);

int PluginVarPrepare(Variation* vp)
{
	vp->var.denom = 0.5 / log(vp->var.pre_log_base);
	
    return 1;
}
int PluginVarCalc(Variation* vp)
{
	const double x_in = FTx;
	const double y_in = FTy;
	const double z_in = FTz;

	FTx = vp->vvar * log(x_in * x_in + y_in * y_in) * vp->var.denom;
	FTy = vp->vvar * atan2(y_in, x_in);
	FTz = vp->vvar * z_in;
	
    return 1;
}
