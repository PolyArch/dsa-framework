#include "ssdfg.h"
#include "serialization.h"

//Boost Includes
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/export.hpp>


template<class Archive>
void SSDfgEdge::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_NVP(_ID);
  ar & BOOST_SERIALIZATION_NVP(_ssdfg);
  ar & BOOST_SERIALIZATION_NVP(_def);
  ar & BOOST_SERIALIZATION_NVP(_use);
  ar & BOOST_SERIALIZATION_NVP(_etype);
  ar & BOOST_SERIALIZATION_NVP(_l);
  ar & BOOST_SERIALIZATION_NVP(_r);
}

template<class Archive>
void SSDfgOperand::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_NVP(edges);
  ar & BOOST_SERIALIZATION_NVP(imm);
}

template<class Archive>
void SSDfgNode::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_NVP(_ssdfg);
  ar & BOOST_SERIALIZATION_NVP(_ID);
  ar & BOOST_SERIALIZATION_NVP(_name);
  ar & BOOST_SERIALIZATION_NVP(_ops);
  ar & BOOST_SERIALIZATION_NVP(_inc_edge_list);
  ar & BOOST_SERIALIZATION_NVP(_uses);
  ar & BOOST_SERIALIZATION_NVP(_min_lat);
  ar & BOOST_SERIALIZATION_NVP(_sched_lat);
  ar & BOOST_SERIALIZATION_NVP(_max_thr);
  ar & BOOST_SERIALIZATION_NVP(_group_id);
}

template<class Archive>
void CtrlBits::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_NVP(_bits);
}

template<class Archive>
void SSDfgInst::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SSDfgNode);
  ar & BOOST_SERIALIZATION_NVP(_predInv);
  ar & BOOST_SERIALIZATION_NVP(_isDummy);
  ar & BOOST_SERIALIZATION_NVP(_imm_slot);
  ar & BOOST_SERIALIZATION_NVP(_subFunc);
  ar & BOOST_SERIALIZATION_NVP(_ctrl_bits);
  ar & BOOST_SERIALIZATION_NVP(_reg);
  ar & BOOST_SERIALIZATION_NVP(_reg_32);
  ar & BOOST_SERIALIZATION_NVP(_reg_16);
  ar & BOOST_SERIALIZATION_NVP(_reg_8);
  ar & BOOST_SERIALIZATION_NVP(_imm);
  ar & BOOST_SERIALIZATION_NVP(_ssinst);
}

template<class Archive>
void SSDfgIO::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SSDfgNode);
  ar & BOOST_SERIALIZATION_NVP(vec_);
}

template<class Archive>
void SSDfgInput::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SSDfgIO);
}

template<class Archive>
void SSDfgOutput::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SSDfgIO);
}

template<class Archive>
void SSDfgVec::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_NVP(_name);
  ar & BOOST_SERIALIZATION_NVP(_ID);
  ar & BOOST_SERIALIZATION_NVP(_ssdfg);
  ar & BOOST_SERIALIZATION_NVP(_group_id);
  ar & BOOST_SERIALIZATION_NVP(_port_width);
  ar & BOOST_SERIALIZATION_NVP(_vp_len);
}

template<class Archive>
void SSDfgVecInput::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SSDfgVec);
  ar & BOOST_SERIALIZATION_NVP(_inputs);
}

template<class Archive>
void SSDfgVecOutput::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SSDfgVec);
  ar & BOOST_SERIALIZATION_NVP(_outputs);
}

template<class Archive>
void GroupProp::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_NVP(is_temporal);
}

template<class Archive>
void SSDfg::serialize(Archive & ar, const unsigned version) {
  ar & BOOST_SERIALIZATION_NVP(_nodes);
  ar & BOOST_SERIALIZATION_NVP(_insts);
  ar & BOOST_SERIALIZATION_NVP(_inputs);
  ar & BOOST_SERIALIZATION_NVP(_outputs);
  ar & BOOST_SERIALIZATION_NVP(_orderedInsts);
  ar & BOOST_SERIALIZATION_NVP(_orderedInstsGroup);
  ar & BOOST_SERIALIZATION_NVP(_vecInputs);
  ar & BOOST_SERIALIZATION_NVP(_vecOutputs);
  ar & BOOST_SERIALIZATION_NVP(_edges);
  ar & BOOST_SERIALIZATION_NVP(removed_edges);
  ar & BOOST_SERIALIZATION_NVP(_vecInputGroups);
  ar & BOOST_SERIALIZATION_NVP(_vecOutputGroups);
  ar & BOOST_SERIALIZATION_NVP(_groupProps);
  ar & BOOST_SERIALIZATION_NVP(dummy_map);
  ar & BOOST_SERIALIZATION_NVP(dummys_per_port);
  ar & BOOST_SERIALIZATION_NVP(dummies);
  ar & BOOST_SERIALIZATION_NVP(dummiesOutputs);
}

//Boost Stuff
BOOST_CLASS_EXPORT_GUID(SSDfgEdge,     "SSDfgEdge");
BOOST_CLASS_EXPORT_GUID(SSDfgNode,     "SSDfgNode")
BOOST_CLASS_EXPORT_GUID(SSDfgInst,     "SSDfgInst");
BOOST_CLASS_EXPORT_GUID(SSDfgInput,    "SSDfgInput")
BOOST_CLASS_EXPORT_GUID(SSDfgOutput,   "SSDfgOutput")
BOOST_CLASS_EXPORT_GUID(SSDfgVec,      "SSDfgVec")
BOOST_CLASS_EXPORT_GUID(SSDfgVecInput, "SSDfgVecInput")
BOOST_CLASS_EXPORT_GUID(SSDfgVecOutput,"SSDfgVecOutput")
BOOST_CLASS_EXPORT_GUID(SSDfg,         "SSDfg")

SERIALIZABLE(SSDfgEdge);
SERIALIZABLE(SSDfgNode);
SERIALIZABLE(SSDfgInst);
SERIALIZABLE(SSDfgInput);
SERIALIZABLE(SSDfgOutput);
SERIALIZABLE(SSDfgVec);
SERIALIZABLE(SSDfgVecInput);
SERIALIZABLE(SSDfgVecOutput);
SERIALIZABLE(SSDfg);
