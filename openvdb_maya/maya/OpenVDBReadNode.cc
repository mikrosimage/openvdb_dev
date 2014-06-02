///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2013 DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

/// @author FX R&D OpenVDB team

#include <fnmatch.h>

#include "OpenVDBPlugin.h"
#include <openvdb_maya/OpenVDBData.h>
#include <openvdb/io/Stream.h>

#include <maya/MFnTypedAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MTime.h>

namespace mvdb = openvdb_maya;


////////////////////////////////////////


struct OpenVDBReadNode : public MPxNode
{
    OpenVDBReadNode() {}
    virtual ~OpenVDBReadNode() {}

    virtual MStatus compute(const MPlug& plug, MDataBlock& data);

    static void* creator();
    static MStatus initialize();
    static MObject aVdbFilePath;
    static MObject aVdbOutput;
    static MTypeId id;
    static MObject aVdbAllGridNames;
    static MObject aResolvedVdbFilePath;
    static MObject aVdbSequence;
    static MObject aTime;
    static MObject aPadding;
};


MTypeId OpenVDBReadNode::id(0x00108A51);
MObject OpenVDBReadNode::aVdbFilePath;
MObject OpenVDBReadNode::aVdbOutput;
MObject OpenVDBReadNode::aVdbAllGridNames;
MObject OpenVDBReadNode::aResolvedVdbFilePath;
MObject OpenVDBReadNode::aVdbSequence;
MObject OpenVDBReadNode::aTime;
MObject OpenVDBReadNode::aPadding;

namespace {
    mvdb::NodeRegistry registerNode("OpenVDBRead", OpenVDBReadNode::id,
        OpenVDBReadNode::creator, OpenVDBReadNode::initialize);
}

std::string repeat( const std::string &word, int times ) {
   std::string result ;
   result.reserve(times*word.length()); // avoid repeated reallocation
   for ( int a = 0 ; a < times ; a++ ) 
      result += word ;
   return result ;
}

std::string zfill(int num, int pad)
{
    std::ostringstream ss;
    ss << std::setw(pad) << std::setfill('0') << num;
    return ss.str();
}


////////////////////////////////////////


void* OpenVDBReadNode::creator()
{
        return new OpenVDBReadNode();
}


MStatus OpenVDBReadNode::initialize()
{
    MStatus stat;
    MFnTypedAttribute tAttr;
    MFnNumericAttribute nAttr;
    MFnUnitAttribute uAttr;

    MFnStringData fnStringData;
    MObject defaultStringData = fnStringData.create("");

    // Setup the input attributes

    aVdbFilePath = tAttr.create("VdbFilePath", "file", MFnData::kString, defaultStringData, &stat);
    if (stat != MS::kSuccess) return stat;

    // tAttr.setConnectable(false);
    stat = addAttribute(aVdbFilePath);
    if (stat != MS::kSuccess) return stat;

    // Setup the output attributes

    aVdbOutput = tAttr.create("VdbOutput", "vdb", OpenVDBData::id, MObject::kNullObj, &stat);
    if (stat != MS::kSuccess) return stat;

    tAttr.setWritable(false);
    tAttr.setStorable(false);
    stat = addAttribute(aVdbOutput);
    if (stat != MS::kSuccess) return stat;

    aVdbAllGridNames = tAttr.create("VdbAllGridNames", "allgrids", MFnData::kString, defaultStringData, &stat);
    if (stat != MS::kSuccess) return stat;
    tAttr.setHidden(true);
    stat = addAttribute(aVdbAllGridNames);
    if (stat != MS::kSuccess) return stat;

    // sequence
    aResolvedVdbFilePath = tAttr.create("ResolvedVdbFilePath", "rfile", MFnData::kString, defaultStringData, &stat);
    if (stat != MS::kSuccess) return stat;
    stat = addAttribute(aResolvedVdbFilePath);
    if (stat != MS::kSuccess) return stat;

    aVdbSequence = nAttr.create("vdbSequence", "vdbseq", MFnNumericData::kBoolean, false, &stat);
    stat = addAttribute(aVdbSequence);
    if (stat != MS::kSuccess) return stat;

    aTime = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0, &stat);
    stat = addAttribute(aTime);
    if (stat != MS::kSuccess) return stat;

    aPadding = nAttr.create("padding", "pd", MFnNumericData::kByte, 4, &stat);
    stat = addAttribute(aPadding);
    if (stat != MS::kSuccess) return stat;

    // Set the attribute dependencies
    stat = attributeAffects(aVdbFilePath, aResolvedVdbFilePath);
    if (stat != MS::kSuccess) return stat;
    stat = attributeAffects(aVdbSequence, aResolvedVdbFilePath);
    if (stat != MS::kSuccess) return stat;
    stat = attributeAffects(aTime, aResolvedVdbFilePath);
    if (stat != MS::kSuccess) return stat;
    stat = attributeAffects(aPadding, aResolvedVdbFilePath);
    if (stat != MS::kSuccess) return stat;
    
    stat = attributeAffects(aVdbFilePath, aVdbOutput);
    if (stat != MS::kSuccess) return stat;
    stat = attributeAffects(aVdbSequence, aVdbOutput);
    if (stat != MS::kSuccess) return stat;
    stat = attributeAffects(aTime, aVdbOutput);
    if (stat != MS::kSuccess) return stat;
    stat = attributeAffects(aPadding, aVdbOutput);
    if (stat != MS::kSuccess) return stat;

    //stat = attributeAffects(aResolvedVdbFilePath, aVdbOutput);
    //stat = attributeAffects(aVdbFilePath, aVdbOutput);
    //if (stat != MS::kSuccess) return stat;
    stat = attributeAffects(aVdbFilePath, aVdbAllGridNames);
    if (stat != MS::kSuccess) return stat;

    return MS::kSuccess;
}

////////////////////////////////////////

std::string resolvedPath(MString &path, int time, int padding, bool seq)
{
    std::string result = path.asChar();

    if(seq == true)
    {
        std::string pattern = "*." + repeat("[0-9]", padding) + ".vdb";
        if( fnmatch(pattern.c_str(), result.c_str(), 0) == 0 )
        {
            std::string repstr = zfill(time, padding) + ".vdb";
            result.replace(result.length() - repstr.length(), result.length(), repstr);
        }
    }
        
    return result;
}


MStatus OpenVDBReadNode::compute(const MPlug& plug, MDataBlock& data)
{

    if (plug == aResolvedVdbFilePath) {
        
        MStatus status;
        MDataHandle filePathHandle = data.inputValue (aVdbFilePath, &status);
        if (status != MS::kSuccess) return status;
        MDataHandle vdbSequenceHandle = data.inputValue (aVdbSequence, &status);
        if (status != MS::kSuccess) return status;
        MDataHandle timeHandle = data.inputValue (aTime, &status);
        if (status != MS::kSuccess) return status;
        MDataHandle paddingHandle = data.inputValue (aPadding, &status);
        if (status != MS::kSuccess) return status;
    
        std::string fp = resolvedPath(filePathHandle.asString(), (int)timeHandle.asTime().value(),
                paddingHandle.asChar(), vdbSequenceHandle.asBool());

        MDataHandle resolvedFilePathHandle = data.outputValue (aResolvedVdbFilePath, &status);
        if (status != MS::kSuccess) return status;

        resolvedFilePathHandle.setString(MString(fp.c_str()));

        return data.setClean(plug);
    }
    else if (plug == aVdbOutput) {

        MStatus status;
        MDataHandle filePathHandle = data.inputValue (aVdbFilePath, &status);
        if (status != MS::kSuccess) return status;
        MDataHandle vdbSequenceHandle = data.inputValue (aVdbSequence, &status);
        if (status != MS::kSuccess) return status;
        MDataHandle timeHandle = data.inputValue (aTime, &status);
        if (status != MS::kSuccess) return status;
        MDataHandle paddingHandle = data.inputValue (aPadding, &status);
        if (status != MS::kSuccess) return status;
        
        if(filePathHandle.asString().length() == 0)
        {
            return MS::kFailure;
        }

        std::string fp = resolvedPath(filePathHandle.asString(), (int)timeHandle.asTime().value(),
                paddingHandle.asChar(), vdbSequenceHandle.asBool());

        std::ifstream ifile(fp.c_str(), std::ios_base::binary);
        
        if(!ifile)
        {
            return MS::kFailure;
        }

        openvdb::GridPtrVecPtr grids = openvdb::io::Stream(ifile).getGrids();

        if (!grids->empty()) {
            MFnPluginData outputDataCreators;
            outputDataCreators.create(OpenVDBData::id, &status);
            if (status != MS::kSuccess) return status;

            OpenVDBData* vdb = static_cast<OpenVDBData*>(outputDataCreators.data(&status));
            if (status != MS::kSuccess) return status;

            vdb->insert(*grids);

            MDataHandle outHandle = data.outputValue(aVdbOutput);
            outHandle.set(vdb);

            MString names;
            for(int i=0; i<(*grids).size(); i++)
            {
                if( i > 0 )
                {
                    names += " ";
                }
                names += (*grids)[i]->getName().c_str();
            }
           
            MDataHandle outHandle2 = data.outputValue(aVdbAllGridNames);
            //outHandle2.set(names);
            outHandle2.setString(names);

        }

        return data.setClean(plug);
    }

    return MS::kUnknownParameter;
}

// Copyright (c) 2012-2013 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
