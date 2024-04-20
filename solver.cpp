#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <ceres/ceres.h>
#include <ceres/loss_function.h>

#include <QList>
#include <QVector>
#include <QDebug>

struct sphereFit
{
    sphereFit(double raw_x, double raw_y, double raw_z)
    {
        raw[0] = raw_x;
        raw[1] = raw_y;
        raw[2] = raw_z;
    }

    template <typename T>
    bool operator()(
        const T *const k,               // 9: calib parameters
        T *residuals) const
    {
        // second order
        // cor = a * raw^2 + b * raw + c
        T cor_x = k[0] * (T)raw[0] * (T)raw[0] + k[1] * (T)raw[0] + k[2];
        T cor_y = k[3] * (T)raw[1] * (T)raw[1] + k[4] * (T)raw[1] + k[5];
        T cor_z = k[6] * (T)raw[2] * (T)raw[2] + k[7] * (T)raw[2] + k[8];

        T sphere = cor_x * cor_x + cor_y * cor_y + cor_z * cor_z;

        residuals[0] = sphere - 1.0;
        return true;
    }

    static ceres::CostFunction *Create(double raw_x, double raw_y, double raw_z)
    {
        return (new ceres::AutoDiffCostFunction<sphereFit, 1, 9>(
            new sphereFit(raw_x, raw_y, raw_z))
            );
    }

    double raw[3];
};


// sphere fitting

int solve(QList<QList<double> > &dataSet, double t0, double t1, QVector<double> &k)
{
    ceres::Problem problem;
    ceres::Covariance::Options covOptions;

    double cal[9]={0.0, 1.0,0.0, 0.0,1.0,0.0, 0.0,1.0,0.0};  // 3x  y=axx+bx+c

    ceres::LossFunction *loss = new ceres::HuberLoss(1.0);

    problem.AddParameterBlock(&cal[0],9);

    int n=0;
    for(const auto &i:dataSet)
    {
        ceres::CostFunction *cost_function;
        auto time = i.at(0);
        if(t0<time && time<t1)
        {
            cost_function = sphereFit::Create(i.at(1), i.at(2), i.at(3));
            problem.AddResidualBlock(cost_function, loss, &cal[0]);
            n++;
        }
    }

    if(n)
    {
        ceres::Solver::Options solOptions;
        solOptions.max_num_iterations = 1000;
        solOptions.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY;
        solOptions.sparse_linear_algebra_library_type = ceres::SUITE_SPARSE;
        solOptions.num_threads = 1;
        solOptions.minimizer_progress_to_stdout = true;

        ceres::Solver::Summary summary;
        ceres::Solve(solOptions, &problem, &summary);

        qInfo() << summary.FullReport().c_str();

        k.clear();
        if (summary.IsSolutionUsable())
        {
            for(int i=0;i<9;i++)
            {
                k.append(cal[i]);
            }
            return 1;
        }
    }

    return 0;
}
