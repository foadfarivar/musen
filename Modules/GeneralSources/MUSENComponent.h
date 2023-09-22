/* Copyright (c) 2013-2020, MUSEN Development Team. All rights reserved.
   This file is part of MUSEN framework http://msolids.net/musen.
   See LICENSE file for license and warranty information. */

#pragma once

#include "SystemStructure.h"

class CMusenComponent
{
protected:
	CSystemStructure* m_pSystemStructure{ nullptr };

public:
	virtual ~CMusenComponent() = default;

	virtual void SetSystemStructure(CSystemStructure* _pSystemStructure) { m_pSystemStructure = _pSystemStructure; }

	// Loads all data from system structure's storage. MUST be called AFTER CSystemStructure::LoadFromFile()
	virtual void LoadConfiguration() {}
	// Saves all data to system structure's storage. MUST be called BEFORE CSystemStructure::SaveToFile()
	virtual void SaveConfiguration() {}
};
