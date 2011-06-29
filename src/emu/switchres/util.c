/*
 *
 *   SwitchRes 2010 Chris Kennedy <c@groovy.org>
 * 
 *   SwitchRes is a modeline generator based off of Calamity's 
 *   Methods of generating modelines
 *
 */

#include "emu.h"
#include "emuopts.h"
#include <stdio.h>
#include <stdarg.h>
#include "math.h"

double Normalize(double a, double b) {
	double c, e;
	int d;

	c = (double)a / (double)b;
	d = a / b;
	e = c - (double)d;
	if (e > 0.0)
		d++;
	a = d * b;

	return a;
}

