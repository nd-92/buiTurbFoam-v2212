#include "readTimeControls.H"
#include "compressibleCourantNo.H"
#include "setDeltaT.H"

runTime++;
Info << "\nTime = " << runTime.value() << endl;

// Low storage Runge-Kutta time integration
forAll(beta, i)
{
    // Solve the approximate Riemann problem for this time step
    dbnsFlux.computeFlux();

    // Time integration
    solve(                                                                                                                                                                                            //
        1.0 / beta[i] * fvm::ddt(rho)                                                                                                                                                                 //
        + fvc::div(dbnsFlux.rhoFlux()));                                                                                                                                                              //
    solve(                                                                                                                                                                                            //
        1.0 / beta[i] * fvm::ddt(rhoU)                                                                                                                                                                //
        + fvc::div(dbnsFlux.rhoUFlux())                                                                                                                                                               //
        - fvc::laplacian(turbulence->muEff(), U)                                                                                                                                                      //
        - fvc::div(turbulence->muEff() * dev2(Foam::T(fvc::grad(U)))));                                                                                                                               //
    solve(                                                                                                                                                                                            //
        1.0 / beta[i] * fvm::ddt(rhoE)                                                                                                                                                                //
        + fvc::div(dbnsFlux.rhoEFlux())                                                                                                                                                               //
        - fvc::div((fvc::interpolate(turbulence->muEff()) * mesh.magSf() * fvc::snGrad(U) + fvc::dotInterpolate(mesh.Sf(), turbulence->muEff() * dev2(Foam::T(fvc::grad(U))))) & fvc::interpolate(U)) //
        - fvc::laplacian(turbulence->alphaEff(), e));                                                                                                                                                 //

    // Compute U
    // Applying damping here may help stability
    rho = (rho * acousticBlending) - (rho_inf * (acousticBlending - 1));
    rhoU = (rhoU * acousticBlending) - (rho_inf * U_inf * (acousticBlending - 1));
    U = rhoU / rho;
    U.correctBoundaryConditions();
    rhoU.boundaryFieldRef() == rho.boundaryField() * U.boundaryField();

    // Calculate internal energy
    e = ((rhoE / rho) - (0.5 * magSqr(U)));
    e.correctBoundaryConditions();

    // Correct thermodynamics
    thermo.correct();
    // Applying damping here may help stability
    T = (T * acousticBlending) - (T_inf * (acousticBlending - 1));
    rhoE.boundaryFieldRef() == rho.boundaryField() * (e.boundaryField() + 0.5 * magSqr(U.boundaryField()));

    // Compute p from rho
    // Applying damping here may help stability
    p.ref() = ((rho / thermo.psi()) * acousticBlending) - (p_inf * (acousticBlending - 1));
    p.correctBoundaryConditions();
    rho.boundaryFieldRef() == thermo.psi().boundaryField() * p.boundaryField();

    // Correct turbulence fields
    turbulence->correct();
}