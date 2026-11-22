#include "geom/Version.h"

#include <doctest/doctest.h>
#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>

TEST_CASE("geom version is 0.1.0") {
    const auto v = Geom::version();
    CHECK_FALSE(v.empty());
    CHECK(v == "0.1.0");
}

TEST_CASE("Eigen sparse SimplicialLDLT solves a 2x2 SPD system") {
    // A = [4 1; 1 3], b = [1; 2]  ->  x = [1/11; 7/11]
    Eigen::SparseMatrix<double> a(2, 2);
    a.insert(0, 0) = 4.0;
    a.insert(0, 1) = 1.0;
    a.insert(1, 0) = 1.0;
    a.insert(1, 1) = 3.0;
    a.makeCompressed();

    Eigen::VectorXd b(2);
    b << 1.0, 2.0;

    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
    solver.compute(a);
    REQUIRE(solver.info() == Eigen::Success);

    const Eigen::VectorXd x = solver.solve(b);
    REQUIRE(solver.info() == Eigen::Success);
    CHECK(x(0) == doctest::Approx(1.0 / 11.0).epsilon(1e-12));
    CHECK(x(1) == doctest::Approx(7.0 / 11.0).epsilon(1e-12));
}
