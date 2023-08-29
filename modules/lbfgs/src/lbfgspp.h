#ifndef MLBFGSSolver_H
#define MLBFGSSolver_H

#include "core/error/error_macros.h"
#include "core/object/gdvirtual.gen.inc"
#include "core/object/ref_counted.h"
#include "core/object/script_language.h"

#include "../thirdparty/LBFGSpp/include/LBFGS.h"
#include "thirdparty/eigen/Eigen/Core"

#include <functional>
#include <iostream>

class LBFGSSolver : public RefCounted {
	GDCLASS(LBFGSSolver, RefCounted);

protected:
	static void _bind_methods();
	GDVIRTUAL2R(double, _call_operator, const TypedArray<double> &, TypedArray<double>);

	std::function<double(const Eigen::VectorXd &p_x, Eigen::VectorXd &r_grad)> operator_pointer;

	double operator_call(const TypedArray<double> &p_x, TypedArray<double> &r_grad);

public:
	LBFGSSolver();
	double native_operator(const Eigen::VectorXd &r_x, Eigen::VectorXd &r_grad);
	double call_operator(const TypedArray<double> &p_x, TypedArray<double> &r_grad);
	static Eigen::VectorXd godot_to_eigen(const TypedArray<double> &p_array);
	static TypedArray<double> eigen_to_godot(const Eigen::VectorXd &p_vector);
	Array minimize(const TypedArray<double> &p_x,
			const double &p_fx);
};

#endif // MLBFGSSolver_H
