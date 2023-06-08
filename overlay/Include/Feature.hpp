#pragma once

class C_FeatureBase
{
public:
	virtual ~C_FeatureBase() = default;
	virtual void OnTick() {}
	virtual void OnRender() {}
private:
};

class C_FeatureManagerBase
{
public:
	virtual ~C_FeatureManagerBase() = default;
	virtual void OnTick() {}
	virtual void OnRender() {}

	void Add(C_FeatureBase* pFeature) { m_Features.emplace_back(pFeature); }
protected:
	std::vector<C_FeatureBase*> m_Features{};
private:
};
