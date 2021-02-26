/* Copyright (c) 2013-2020, MUSEN Development Team. All rights reserved.
   This file is part of MUSEN framework http://msolids.net/musen.
   See LICENSE file for license and warranty information. */

#include "ModelPWPopovJKR.h"

CModelPWPopovJKR::CModelPWPopovJKR()
{
	m_name = "Popov-JKR";
	m_uniqueKey = "5048D3D96D3843949F5B427DF9FCCEDF";
	m_helpFileName = "/Contact Models/PopovJKR.pdf";
}

void CModelPWPopovJKR::CalculatePWForce(double _time, double _timeStep, size_t _iWall, size_t _iPart, const SInteractProps& _interactProp, SCollision* _pCollision) const
{
	const CVector3 vRc          = CPU_GET_VIRTUAL_COORDINATE(Particles().Coord(_iPart)) - _pCollision->vContactVector;
	const double   dRc          = vRc.Length();
	const CVector3 nRc          = vRc / dRc; // = vRc.Normalized()
	const CVector3 vVelDueToRot = !Walls().RotVel(_iWall).IsZero() ? (_pCollision->vContactVector - Walls().RotCenter(_iWall)) * Walls().RotVel(_iWall) : CVector3{ 0 };

	// relative velocity (normal and tangential)
	const CVector3 vRelVel       =  Particles().Vel(_iPart) - Walls().Vel(_iWall) + vVelDueToRot + nRc * Particles().AnglVel(_iPart) * Particles().Radius(_iPart);
	const double   dRelVelNormal = DotProduct(Walls().NormalVector(_iWall), vRelVel);
	const CVector3 vRelVelNormal = dRelVelNormal * Walls().NormalVector(_iWall);
	const CVector3 vRelVelTang   = vRelVel - vRelVelNormal;

	// normal and tangential overlaps
	const double dNormalOverlap =  Particles().Radius(_iPart) - dRc;
	if (dNormalOverlap < 0) return;
	const CVector3 vDeltaTangOverlap = vRelVelTang * _timeStep;

	const double Kn = 2 * _interactProp.dEquivYoungModulus * std::sqrt(Particles().Radius(_iPart) * dNormalOverlap);
	const double dDampingForce = 1.8257 * _interactProp.dAlpha * dRelVelNormal * std::sqrt(Kn *  Particles().Mass(_iPart));

	const double dAdhForce = 3. / 2. * _interactProp.dEquivSurfaceTension * PI * Particles().Radius(_iPart);
	const double dSadh = std::pow(0.4626 * std::pow(_interactProp.dEquivSurfaceTension / _interactProp.dEquivYoungModulus, 2) * Particles().Radius(_iPart), 1. / 3.);
	const double dNormalForce = dAdhForce * (-1 + 0.12 * std::pow(dNormalOverlap / dSadh + 1, 5. / 3.))	* std::abs(DotProduct(nRc, Walls().NormalVector(_iWall)));

	// increment of tangential force with damping
	const double Kt = 8 * _interactProp.dEquivShearModulus * std::sqrt(Particles().Radius(_iPart) * dNormalOverlap);
	const CVector3 vDampingTangForce = vRelVelTang * (1.8257 * _interactProp.dAlpha * std::sqrt(Kt *  Particles().Mass(_iPart)));

	// rotate old tangential force
	CVector3 vTangOverlap = _pCollision->vTangOverlap - Walls().NormalVector(_iWall) * DotProduct(Walls().NormalVector(_iWall), _pCollision->vTangOverlap);
	if (vTangOverlap.IsSignificant())
		vTangOverlap = vTangOverlap * _pCollision->vTangOverlap.Length() / vTangOverlap.Length();

	_pCollision->vTangOverlap = vTangOverlap + vDeltaTangOverlap;
	CVector3 vTangForce = -Kt * _pCollision->vTangOverlap;
	// check slipping condition
	const double dNewTangForce = vTangForce.Length();
	if (dNewTangForce > _interactProp.dSlidingFriction * std::abs(dNormalForce))
	{
		vTangForce = vTangForce * (_interactProp.dSlidingFriction * std::abs(dNormalForce) / dNewTangForce);
		_pCollision->vTangOverlap = vTangForce / -Kt;
	}
	else
		_pCollision->vTangForce = vTangForce + vDampingTangForce;

	// calculate Rolling friction
	const CVector3 vRollingTorque =  Particles().AnglVel(_iPart).IsSignificant() ?  Particles().AnglVel(_iPart) * (-_interactProp.dRollingFriction * std::abs(dNormalForce) *  Particles().Radius(_iPart) /  Particles().AnglVel(_iPart).Length()) : CVector3{ 0 };

	// calculate moment
	const CVector3 vMoment = Walls().NormalVector(_iWall) * _pCollision->vTangForce * - Particles().Radius(_iPart) + vRollingTorque;

	// add result to the arrays
	_pCollision->vTotalForce = _pCollision->vTangForce + (dNormalForce + dDampingForce) * Walls().NormalVector(_iWall);
	_pCollision->vResultMoment1 = vMoment;
}
