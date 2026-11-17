#include <prometheus/modeller.hpp>

#include <armadillo>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(Dependencies, ArmadilloAndJsonWork) {
    const arma::mat matrix = {{1.0, 2.0}, {3.0, 4.0}};
    const nlohmann::json result = {{"sum", arma::accu(matrix)}};

    EXPECT_DOUBLE_EQ(result.at("sum").get<double>(), 10.0);
}

TEST(Modeller, ExposesModuleApis) {
    const prometheus::Line line({1.0, 2.0, 3.0}, {1.0, 0.0, 0.0});

    EXPECT_DOUBLE_EQ(line.eval(2.0)[0], 3.0);
    EXPECT_TRUE(prometheus::topology::is_null(prometheus::topology::NULL_ID));
    EXPECT_FALSE(prometheus::topology::is_null(0));
}
