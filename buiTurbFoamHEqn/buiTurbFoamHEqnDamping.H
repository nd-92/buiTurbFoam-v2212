#include "readTimeControls.H"
#include "compressibleCourantNo.H"
#include "setDeltaT.H"

runTime++;
Info << "\nTime = " << runTime.value() << "\n";

// Low storage Runge-Kutta time integration
// forAll(beta, i)
for (uint8_t i = 0; i < 4; i++)
{
    // Solve the approximate Riemann problem for this time step
    dbnsFlux.computeFlux();

    // Time integration
    solve(                               //
        1 / beta[i] * fvm::ddt(rho)      //
        + fvc::div(dbnsFlux.rhoFlux())); //

    solve(                                     //
        1 / beta[i] * fvm::ddt(rhoU)           //
        + fvc::div(dbnsFlux.rhoUFlux())        //
        + fvc::div(turbulence->devRhoReff())); //

    solve(
        1.0 / beta[i] * fvm::ddt(rhoE)                //
        + fvc::div(dbnsFlux.rhoEFlux())               //
        + fvc::div(turbulence->devRhoReff() & U)      //
        - fvc::laplacian(turbulence->alphaEff(), h)); //

    // Apply damping to conservative terms
    rho = (rho * acousticBlending) - (rho_inf * (acousticBlending - 1));
    rhoU = (rhoU * acousticBlending) - (rho_inf * U_inf * (acousticBlending - 1));
    rhoE = (rhoE * acousticBlending) - (rho_inf * e_inf * (acousticBlending - 1));

    // Compute U
    U = rhoU / rho;
    U.correctBoundaryConditions();
    rhoU.boundaryFieldRef() == rho.boundaryField() * U.boundaryField();
    // U = (U * acousticBlending) - (U_inf * (acousticBlending - 1));

    // Calculate enthalpy
    h = thermo.gamma() * ((rhoE / rho) - (0.5 * magSqr(U)));
    h.correctBoundaryConditions();
    // e = (e * acousticBlending) - (e_inf * (acousticBlending - 1));

    // Correct thermodynamics
    thermo.correct();
    // T = (T * acousticBlending) - (T_inf * (acousticBlending - 1));
    // rhoE.boundaryFieldRef() == rho.boundaryField() * (e.boundaryField() + 0.5 * magSqr(U.boundaryField()));

    // Compute p from rho
    p = rho * (thermo.Cp() - thermo.Cv()) * thermo.T();
    // p.ref() = ((rho / thermo.psi()) * acousticBlending) - (p_inf * (acousticBlending - 1));
    p.correctBoundaryConditions();
    // rho.boundaryFieldRef() == thermo.psi().boundaryField() * p.boundaryField();

    // Correct turbulence fields
    turbulence->correct();
    if (turbulenceDamping == true)
    {
        turbulence->muEff() = (turbulence->muEff() * acousticBlending) - (muEff_inf * (acousticBlending - 1));
        turbulence->alphaEff() = (turbulence->alphaEff() * acousticBlending) - (alphaEff_inf * (acousticBlending - 1));
    }
}