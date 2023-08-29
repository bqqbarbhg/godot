#ifndef MLBFGSSolver_H
#define MLBFGSSolver_H

#include "core/error/error_macros.h"
#include "core/object/gdvirtual.gen.inc"
#include "core/object/ref_counted.h"
#include "core/object/script_language.h"

#include "../thirdparty/LBFGSpp/include/LBFGS.h"
#include "thirdparty/eigen/Eigen/Core"

#include <iostream>
#include <functional>

class LBFGSSolver : public RefCounted {
	GDCLASS(LBFGSSolver, RefCounted);

protected:
	static void _bind_methods() {
		GDVIRTUAL_BIND(_call_operator, "x", "grad");
		ClassDB::bind_method(D_METHOD("minimize", "x", "fx"), &LBFGSSolver::minimize);
	}
	GDVIRTUAL2R(double, _call_operator, const TypedArray<double>&, TypedArray<double>);

    std::function<double(const Eigen::VectorXd &p_x, Eigen::VectorXd &r_grad)> operator_pointer;

    double operator_call(const TypedArray<double> &p_x, TypedArray<double> &r_grad) { 
        return call_operator(p_x, r_grad); 
    };
public:
    LBFGSSolver() {        
        operator_pointer = std::bind(&LBFGSSolver::native_operator, this, std::placeholders::_1, std::placeholders::_2);
    }
	double native_operator(const Eigen::VectorXd &r_x, Eigen::VectorXd &r_grad) {
		TypedArray<double> x = LBFGSSolver::eigen_to_godot(r_x);
		TypedArray<double> grad = LBFGSSolver::eigen_to_godot(r_grad);
		Eigen::VectorXd vec(r_grad.size());
		for (int i = 0; i < r_grad.size(); ++i) {
			vec[i] = r_grad[i];
		}
        double fx = call_operator(x, grad);
        r_grad = godot_to_eigen(grad);
		return fx;
	}
	double call_operator(const TypedArray<double> &p_x, TypedArray<double> &r_grad) {
		double ret = 0;
		if (GDVIRTUAL_CALL(_call_operator, p_x, r_grad, ret)) {
			return ret;
		};
		return 0;
	}
	static Eigen::VectorXd godot_to_eigen(const TypedArray<double> &array) {
        ERR_FAIL_COND_V(!array.size(), Eigen::VectorXd());
		Eigen::VectorXd vector(array.size());
		for (int i = 0; i < array.size(); ++i) {
			vector[i] = array[i];
		}
		return vector;
	}
	static TypedArray<double> eigen_to_godot(const Eigen::VectorXd &vector) {
        ERR_FAIL_COND_V(!vector.size(), TypedArray<double>());
        int size = vector.size();
        TypedArray<double> array;
        array.resize(size);
        for (int i = 0; i < size; ++i) {
            array[i] = vector[i];
        }
        return array;
	};
	Array minimize(const TypedArray<double> &p_x,
			const double &p_fx) {
		LBFGSpp::LBFGSParam<double> param;
		param.epsilon = 1e-6;
		param.max_iterations = 100;

		LBFGSpp::LBFGSSolver<double> solver(param);

		Eigen::VectorXd x = godot_to_eigen(p_x);

		double fx = p_fx;
		int niter = solver.minimize(operator_pointer, x, fx);
		Array ret;
		ret.push_back(niter);
		ret.push_back(eigen_to_godot(x));
		ret.push_back(fx);
		return ret;
	}
};

#endif // MLBFGSSolver_H
