#include "lbfgsbpp.h"
#include "LBFGSpp/Param.h"

Array LBFGSBSolver::minimize(const TypedArray<double> &p_x,
		const double &p_fx, const TypedArray<double> &p_lower_bound, const TypedArray<double> &p_upper_bound) {
	LBFGSpp::LBFGSBParam<double> param;
	param.epsilon = 1e-6;
	param.max_iterations = 100;
	LBFGSpp::LBFGSBSolver<double> solver(param);
	Eigen::VectorXd lower_bound = godot_to_eigen(p_lower_bound);

	Eigen::VectorXd x = godot_to_eigen(p_x);
	Eigen::VectorXd upper_bound = godot_to_eigen(p_upper_bound);

	double fx = p_fx;
	int niter = solver.minimize(operator_pointer, x, fx, lower_bound, upper_bound);
	Array ret;
	ret.push_back(niter);
	ret.push_back(eigen_to_godot(x));
	ret.push_back(fx);
	return ret;
}

LBFGSBSolver::LBFGSBSolver() {
	operator_pointer = std::bind(&LBFGSBSolver::native_operator, this, std::placeholders::_1, std::placeholders::_2);
}

void LBFGSBSolver::_bind_methods() {
	GDVIRTUAL_BIND(_call_operator, "x", "grad");
	ClassDB::bind_method(D_METHOD("minimize", "x", "fx"), &LBFGSBSolver::minimize);
}

double LBFGSBSolver::native_operator(const Eigen::VectorXd &r_x, Eigen::VectorXd &r_grad) {
	TypedArray<double> x = LBFGSBSolver::eigen_to_godot(r_x);
	TypedArray<double> grad = LBFGSBSolver::eigen_to_godot(r_grad);
	Eigen::VectorXd vec(r_grad.size());
	for (int i = 0; i < r_grad.size(); ++i) {
		vec[i] = r_grad[i];
	}
	double fx = call_operator(x, grad);
	r_grad = godot_to_eigen(grad);
	return fx;
}
double LBFGSBSolver::call_operator(const TypedArray<double> &p_x, TypedArray<double> &r_grad) {
	double ret = 0;
	if (GDVIRTUAL_CALL(_call_operator, p_x, r_grad, ret)) {
		return ret;
	};
	return 0;
}
Eigen::VectorXd LBFGSBSolver::godot_to_eigen(const TypedArray<double> &p_array) {
	ERR_FAIL_COND_V(!p_array.size(), Eigen::VectorXd());
	Eigen::VectorXd vector(p_array.size());
	for (int i = 0; i < p_array.size(); ++i) {
		vector[i] = p_array[i];
	}
	return vector;
}
TypedArray<double> LBFGSBSolver::eigen_to_godot(const Eigen::VectorXd &p_vector) {
	ERR_FAIL_COND_V(!p_vector.size(), TypedArray<double>());
	int size = p_vector.size();
	TypedArray<double> array;
	array.resize(size);
	for (int i = 0; i < size; ++i) {
		array[i] = p_vector[i];
	}
	return array;
};
