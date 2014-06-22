/*
 * ConsensusVariable.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <cace/CaceType.h>
#include <variables/ConsensusVariable.h>
#include <cstdint>
#include <string>

namespace cace
{

	ConsensusVariable::~ConsensusVariable()
	{
	}

	ConsensusVariable::ConsensusVariable(const ConsensusVariable& v)
	{
		this->setAcceptStrategy(v.strategy);
		this->decissionTime = v.decissionTime;
		this->hasValue = v.hasValue;
		this->lamportAge = v.lamportAge;
		this->name = string(v.name);
		this->proposals = v.proposals;
		this->robotID = v.robotID;
		this->type = v.type;
		this->validityTime = v.validityTime;
		this->val = v.val;
		this->arrivalTime = v.arrivalTime;
	}
	ConsensusVariable::ConsensusVariable(string name, acceptStrategy strategy, unsigned long validityTime, int robotID,
											unsigned long decissionTime, unsigned long lamportAge, short type)
	{
		this->hasValue = false;
		this->name = name;
		this->setAcceptStrategy(strategy);
		this->validityTime = validityTime;
		this->robotID = robotID;
		this->decissionTime = decissionTime;
		this->lamportAge = lamportAge;
		this->type = type;
	}

	void ConsensusVariable::update(ConsensusVariable& v)
	{
		this->name = string(v.getName());
		this->val = vector<uint8_t>(v.val);
		this->arrivalTime = v.arrivalTime;
		this->setAcceptStrategy(v.strategy);
		if (v.validityTime != std::numeric_limits<long>::max())
			this->validityTime = v.validityTime;
		if (v.decissionTime != std::numeric_limits<long>::max())
			this->decissionTime = v.decissionTime;
		//Do we need to update the lamport age?
		this->lamportAge = v.lamportAge;
		if (v.type != 0)
			this->type = v.type;
	}
	bool ConsensusVariable::valueEqual(vector<uint8_t>* cmp)
	{
		if (!hasValue && cmp == nullptr)
		{
			return true;
		}
		if (!hasValue && cmp != nullptr)
		{
			return false;
		}
		if (hasValue && cmp == nullptr)
		{
			return false;
		}
		if (val.size() != cmp->size())
		{
			return false;
		}
		for (int i = 0; i < val.size(); i++)
		{
			if (val.at(i) != cmp->at(i))
				return false;
		}
		return true;
	}
	bool ConsensusVariable::believeEqual(ConsensusVariable& v)
	{
		return valueEqual(&v.val) && v.type == type && v.validityTime == validityTime
				&& v.decissionTime == decissionTime;
	}
	bool ConsensusVariable::isAcknowledged(Cace& c)
	{
		//If we have lass believes than active robots -> Inconsistent
		if (proposals.size() < c.getActiveRobots()->size())
		{
			return false;
		}

		//If one robot didn't send an acknowledge -> Inconsistent
		for (auto v : proposals)
		{
			if (lamportAge > v->lamportAge)
				return false;
		}
		return true;
	}

	bool ConsensusVariable::checkConflict(Cace& c)
	{
		for (auto cv : proposals)
		{
			for (int i : (*c.getActiveRobots()))
			{
				if (i == cv->robotID && !valueEqual(&cv->val))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool ConsensusVariable::isAgreed(Cace& c)
	{
		return isAcknowledged(c) && !checkConflict(c);
	}

	string ConsensusVariable::getScope()
	{
		int idx = name.find_last_of('/');
		if (idx == string::npos)
			return "/";

		return name.substr(0, idx) + string("/");
	}

	vector<uint8_t> ConsensusVariable::getValue()
	{
		return val;
	}

	void ConsensusVariable::setValue(vector<uint8_t> value)
	{
		val = value;
		hasValue = true;
	}

	string& ConsensusVariable::getName()
	{
		return name;
	}
	void ConsensusVariable::setName(string name)
	{
		this->name = name;
	}

	short ConsensusVariable::getType()
	{
		return type;
	}
	void ConsensusVariable::setType(short t)
	{
		type = t;
	}

	int ConsensusVariable::getRobotID()
	{
		return robotID;
	}
	void ConsensusVariable::setRobotID(int id)
	{
		this->robotID = id;
	}
	unsigned long ConsensusVariable::getArrivalTime()
	{
		return arrivalTime;
	}
	void ConsensusVariable::setArrivalTime(unsigned long at)
	{
		arrivalTime = at;
	}
	unsigned long ConsensusVariable::getDecissionTime()
	{
		return decissionTime;
	}
	void ConsensusVariable::setDecissionTime(unsigned long dt)
	{
		decissionTime = dt;
	}
	unsigned long ConsensusVariable::getValidityTime()
	{
		return validityTime;
	}
	void ConsensusVariable::setValidityTime(unsigned long vt)
	{
		validityTime = vt;
	}
	unsigned long ConsensusVariable::getLamportAge()
	{
		return lamportAge;
	}
	void ConsensusVariable::setLamportAge(unsigned long la)
	{
		lamportAge = la;
	}

	acceptStrategy ConsensusVariable::getAcceptStrategy()
	{
		return strategy;
	}
	void ConsensusVariable::setAcceptStrategy(acceptStrategy aS)
	{
		switch (aS)
		{
			case acceptStrategy::ThreeWayHandShakeElection:
			case acceptStrategy::TwoWayHandShakeElection:
			case acceptStrategy::FireAndForgetElection:
				acceptFunction = &ConsensusVariable::electionAcceptStrategy;
				break;
			case acceptStrategy::ThreeWayHandShakeSet:
			case acceptStrategy::TwoWayHandShakeSet:
			case acceptStrategy::FireAndForgetSet:
				acceptFunction = &ConsensusVariable::listAcceptStrategy;
				break;
			case acceptStrategy::ThreeWayHandShakeLowestID:
			case acceptStrategy::TwoWayHandShakeLowestID:
				acceptFunction = &ConsensusVariable::lowestIDAcceptStrategy;
				break;
			case acceptStrategy::ThreeWayHandShakeMostRecent:
			case acceptStrategy::TwoWayHandShakeMostRecent:
				acceptFunction = &ConsensusVariable::mostRecentAcceptStrategy;
				break;
			default:
				acceptFunction = &ConsensusVariable::defaultAcceptStrategy;
				break;

		}
		strategy = aS;
	}

	bool ConsensusVariable::defaultAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue)
	{
		//if we receive a unknown variable commandedValue != null
		if (commandedValue != nullptr)
		{
			this->setValue(*commandedValue);
		}

		//if(!checkConflict(c)) return false;
		ConsensusVariable* newest = this;
		for (auto cv : proposals)
		{
			//if lamport time is newer or
			if ((cv->lamportAge > newest->lamportAge)
					|| (cv->lamportAge == newest->lamportAge && cv->robotID < newest->robotID))
			{
				newest = &(*cv);
			}
		}
		if (newest != this)
		{
			update(*newest);
			return true;
		}
		return false;
	}
	bool ConsensusVariable::lowestIDAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue)
	{
		if (commandedValue != nullptr)
		{
			this->setValue(*commandedValue);
		}

		//if(!checkConflict(c)) return false;
		ConsensusVariable* prio = this;
		for (auto cv : proposals)
		{
			if (cv->hasValue && cv->robotID < prio->robotID)
			{
				prio = &(*cv);
			}
		}
		if (prio != this)
		{
			update(*prio);
			return true;
		}
		return false;
	}
	bool ConsensusVariable::mostRecentAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue)
	{
		if (commandedValue != nullptr)
		{
			this->setValue(*commandedValue);
		}

		//if(!checkConflict(c)) return false;
		ConsensusVariable* newest = this;
		for (auto cv : proposals)
		{
			if (cv->hasValue && cv->decissionTime > newest->decissionTime)
			{
				newest = &(*cv);
			}
		}
		if (newest != this)
		{
			update(*newest);
			return true;
		}
		return false;
	}
	bool ConsensusVariable::electionAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue)
	{
		if (!hasValue)
			setValue(std::numeric_limits<double>::min());
		return true;
	}

	void ConsensusVariable::acceptProposals(Cace& cace, vector<uint8_t>* value)
	{
		(this->*acceptFunction)(cace, value);
	}

	bool ConsensusVariable::listAcceptStrategy(Cace &c, vector<uint8_t>* commandedValue)
	{
		if (commandedValue != nullptr)
		{
			this->setValue(*commandedValue);
		}

		//if(!checkConflict(c)) return false;
		ConsensusVariable* newest = this;
		for (auto c : proposals)
		{
			//TODO find new data
		}
		if (newest != this)
		{
			update(*newest);
			return true;
		}
		return false;
	}

	string ConsensusVariable::valueAsString()
	{
		if (!hasValue)
			return " ";

		if (type == CaceType::CDouble)
		{
			double t;
			getValue(&t);
			return std::to_string(t) + string("d");
		}
		else if (type == CaceType::CInt)
		{
			int tmp;
			getValue(&tmp);
			return to_string(tmp) + string("i");
		}
		else if (type == CaceType::CString)
		{
			string s;
			getValue(s);
			return s;
		}
		/* else if (Type == RosCS.ConsensusEngine.CaceType.CPoint2)
		 {
		 double x = BitConverter.ToDouble(Value.ToArray(), 0);
		 double y = BitConverter.ToDouble(Value.ToArray(), sizeof(double));
		 return "(" + x + "," + y + ")";
		 }
		 else if (Type == RosCS.ConsensusEngine.CaceType.CIntList)
		 {
		 List<int> l = null;
		 this.GetValue(out l);
		 string ret = "il(";
		 if (l.Count > 0)
		 {
		 for (int i = 0; i < l.Count - 1; i++)
		 {
		 ret += l[i] + "-";
		 }
		 ret += l[l.Count - 1];
		 }
		 ret += ")";
		 return ret;
		 }
		 else if (Type == RosCS.ConsensusEngine.CaceType.CDoubleList)
		 {
		 List<double> l = null;
		 this.GetValue(out l);
		 string ret = "dl(";
		 if (l.Count > 0)
		 {
		 for (int i = 0; i < l.Count - 1; i++)
		 {
		 ret += l[i] + "-";
		 }
		 ret += l[l.Count - 1];
		 }
		 ret += ")";
		 return ret;
		 }
		 else if (Type == RosCS.ConsensusEngine.CaceType.CStringList)
		 {
		 List<string> l = null;
		 this.GetValue(out l);
		 string ret = "sl(";
		 if (l.Count > 0)
		 {
		 for (int i = 0; i < l.Count - 1; i++)
		 {
		 ret += l[i] + "-";
		 }
		 ret += l[l.Count - 1];
		 }
		 ret += ")";
		 return ret;
		 }
		 return Value.ToString();*/
		return " ";
	}

	string ConsensusVariable::toString()
	{
		string ret;
		if (proposals.size() == 0)
		{
			ret = name + "\t";
			ret += valueAsString();
			ret += "\t";
		}
		else
		{
			ret = name + "\t";
			ret += valueAsString();
			ret += "\t{";
			for (auto cv : proposals)
			{
				ret += " (" + std::to_string(cv->robotID) + "-";
				ret += "" + cv->valueAsString();
				ret += ")";
			}
			ret += " }";
		}
		ret += "\tAge: " + to_string(lamportAge);
		ret += "\tType: " + to_string(type);
		ret += "\tValidityTime: " + to_string(validityTime);
		return ret;
	}

	bool ConsensusVariable::getValue(double* out)
	{
		if (type == CaceType::CDouble)
		{
			*out = deserialize<double>(val);
		}
		return type == CaceType::CDouble;
	}
	void ConsensusVariable::setValue(double in)
	{
		val.clear();
		serialize(in, val);
		hasValue = true;
		type = CaceType::CDouble;
	}

	bool ConsensusVariable::getValue(int* out)
	{
		if (type == CaceType::CInt)
		{
			*out = deserialize<int>(val);
		}
		return type == CaceType::CInt;
	}

	void ConsensusVariable::setValue(int in)
	{
		val.clear();
		serialize(in, val);
		hasValue = true;
		type = CaceType::CInt;
	}

	bool ConsensusVariable::getValue(string& out)
	{
		if (type == CaceType::CString)
		{
			out = deserialize<string>(val);
		}
		return type == CaceType::CString;
	}

	void ConsensusVariable::setValue(string* in)
	{
		val.clear();
		serialize(*in, val);
		type = CaceType::CString;
	}

	bool ConsensusVariable::getValue(vector<double>& out)
	{
		if (type == CaceType::CDoubleList)
		{
			out = deserialize<vector<double>>(val);
		}
		return type == CaceType::CDoubleList;
	}

	void ConsensusVariable::setValue(vector<double>* in)
	{
		val.clear();
		serialize(*in, val);
		type = CaceType::CDoubleList;
	}

	bool ConsensusVariable::getValue(vector<int>& out)
	{
		if (type == CaceType::CIntList)
		{
			out = deserialize<vector<int>>(val);
		}
		return type == CaceType::CIntList;
	}

	void ConsensusVariable::setValue(vector<int>* in)
	{
		val.clear();
		serialize(*in, val);
		type = CaceType::CIntList;
	}

	bool ConsensusVariable::getValue(vector<string>& out)
	{
		if (type == CaceType::CStringList)
		{
			out = deserialize<vector<string>>(val);
		}
		return type == CaceType::CStringList;
	}

	void ConsensusVariable::setValue(vector<string>* in)
	{
		val.clear();
		serialize(*in, val);
		type = CaceType::CStringList;
	}

} /* namespace cace */

