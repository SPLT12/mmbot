/*
 * strategy_elliptical.cpp
 *
 *  Created on: 7. 6. 2020
 *      Author: ondra
 */


#include <cmath>
#include "strategy_elliptical.h"
#include "strategy_leveraged_base.tcc"
#include "sgn.h"

std::string_view Elliptical_Calculus::id = "elliptical";

Elliptical_Calculus::Elliptical_Calculus(double width):_width(width) {
}

template<typename Fn>
static double numeric_search(double anchor, double second, Fn &&fn) {
	constexpr double accuracy = 1e-8;
	double min = std::min(anchor,second);
	double max = std::max(anchor,second);
	double sd = anchor == min?-1:1;
	double ref = fn(anchor);
	if (ref == 0) return anchor;
	double md = (min+max)/2;
	while ((max - min) / md > accuracy) {
		double v = fn(md);
		double ml = v * ref * sd;
		if (ml > 0) max = md;
		else if (ml < 0) min = md;
		else return md;
		md = (min+max)/2;
	}
	return md;
}


double Elliptical_Calculus::calcPosValue(double power, double asym, double neutral, double curPrice) {
	double width = this->width(neutral);
	double sq= pow2(width)-pow2(curPrice - neutral);
	if (sq < 0) sq = 0;
	return -power * ((std::sqrt(sq) + asym*(curPrice - neutral))/width - 1);
}

double Elliptical_Calculus::calcPosition(double power, double asym, double neutral, double price) {
	double width = this->width(neutral);
	double sq = pow2(pow2(width)) - pow2(width)*pow2(price - neutral);
	if (sq < 0) return 0;
	sq = std::max(sq, (price-neutral)*1e-10);
	return -power * ((price-neutral)/std::sqrt(sq) - asym/width);
}

double Elliptical_Calculus::calcPower(double neutral, double balance, double asym) {
	return balance/(1+std::abs(asym));
}


double Elliptical_Calculus::calcNeutral(double power, double asym, double position, double curPrice) {
	//neutral * (1 - width) = curPrice
	//neutral = curPrice / (1+width);
	//max = neutral

	double fmax = curPrice*curPrice;
	double min = _width>-1.0?curPrice/(1+_width):fmax;
	double max = _width<1.0?curPrice/(1-_width):fmax;
	double n =  numeric_search(min,max,[=](double n){
		return calcPosition(power, asym, n, curPrice) - position;
	});
	return n;
}

///https://www.desmos.com/calculator/js57nrj2ou

double Elliptical_Calculus::calcPrice0(double neutral_price, double asym) {
	double width = this->width(neutral_price);
	double a2 = pow2(asym);
	return neutral_price*(1+a2)+sgn(asym)*std::sqrt(a2*pow2(width)+pow2(a2)*pow2(width))/(1 + a2);
}


double Elliptical_Calculus::calcPriceFromPosition(double power,	double asym, double neutral, double position) {
	double a = asym, a2 = a*a, k = neutral,	p = power,
			p2 = p*p, y = position,	y2 = y*y,
			y3 = y * y2, w = width(neutral),	y4 = y2*y2,
			w2 = w*w, w3 = w * w2,	w4 = w2*w2,
			w6 = w3*w3,	w5 = w2*w3,	p3 = p * p2,
			a3 = a * a2, p4 = p2*p2,	a4 = a2*a2,
			sa = sgn(p*a/w - y);

	double sq = a4*p4*w2 - 4*a3*p3*w3*y + a2*p4*w2 + 6*a2*p2*w4*y2 - 2*a*p3*w3*y - 4*a*p*w5*y3 + p2*w4*y2 + w6*y4;
	if (sq < 0) return neutral;
	return (a2*k*p2 + sa * std::sqrt(sq) - 2*a*k*p*w*y + k*p2 + k*w2*y2)/(a2*p2 - 2*a*p*w*y + p2 + w2*y2);
}

double Elliptical_Calculus::calcNeutralFromValue(double power, double asym, double neutral, double value, double curPrice) {
	   double pref = calcPosition(power, asym, neutral, curPrice);

		double a = asym, a2 = a*a;
		double c = -value, c2 = c*c;
		double w = width(neutral), w2 = w*w;
		double x=  curPrice;
		double p = power, p2 = p*p, p3 = p2*p, p4 = p2*p2;
		double sq =  a2*p4*w2 - c2*p2*w2 - 2*c*p3*w2;
		if (sq < 0) return neutral;
		double dnm = (a2+1)*p2;
		double cmn = a2*p2*x-a*c*p*w-a*p2*w+p2*x;
		double sqs = std::sqrt(sq);
		double k1 = (-sqs+cmn)/dnm;
		double k2 = (sqs+cmn)/dnm;
		double pk1 =  calcPosition(power, asym, k1, curPrice);
		double pk2 =  calcPosition(power, asym, k2, curPrice);
		if (pk1 * pref > 0) return k1;
		if (pk2 * pref > 0) return k2;
		return neutral;
}

IStrategy::MinMax Elliptical_Calculus::calcRoots(double power, double asym, double neutral, double balance) {
	double min = neutral - width(neutral);
	double max = neutral + width(neutral);
	double mid = calcPrice0(neutral, asym);
	IStrategy::MinMax res;
	auto fn = [=](double v) {
		return calcPosValue(power, asym, neutral, v) - balance;
	};
	res.min = numeric_search(mid, min, fn);
	res.max = numeric_search(mid, max, fn);
	return res;
}

double Elliptical_Calculus::width(double neutral) const {
	return _width * neutral;
}


template class Strategy_Leveraged<Elliptical_Calculus> ;

