#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLStringTokenizer.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>

#include "IgCollection.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <math.h>

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoSeparator.h>

using namespace xercesc;

static const double C = 0.299792458; // speed of light in m/ns

static double D0 = 0.0;
static double D  = 0.0;
static double T0 = 0.0;
static double T  = 0.0;

static std::vector<IgV4d> sHits;

static 
void getHits(void* userdata, SoCallbackAction*,
             const SoPrimitiveVertex* v1,
             const SoPrimitiveVertex* v2)
{
  const SbVec3f& point = v1->getPoint();
 
  double X = point[0];
  double Y = point[1];
  double Z = point[2];
  
  D = sqrt(X*X+Y*Y+Z*Z);
  T = (D-D0)/C + T0;
  D0 = D;
  T0 = T;

  sHits.push_back(IgV4d(T,X,Y,Z));
}

int main(int argc, char* argv[])
{
  SoDB::init();
  SoNodeKit::init();
  
  //SoLineSet::initClass();

  try
  {
    XMLPlatformUtils::Initialize();
  }
  
  catch (const XMLException& toCatch)
  {
    char* message = XMLString::transcode(toCatch.getMessage());
    std::cout<<"Error during initialization! : "<< message <<std::endl;
    XMLString::release(&message);
    return 1;
  }

  if ( argc != 3 )
  {
    std::cout<<"Usage: xml2ig [input xml file]  [output json file]"<<std::endl;
    exit(1);
  }
  
  char* xmlFile = argv[1];
  std::ofstream fout(argv[2]);

  XercesDOMParser* parser = new XercesDOMParser();
  parser->setValidationScheme(XercesDOMParser::Val_Always);

  ErrorHandler* errorHandler = (ErrorHandler*) new HandlerBase();       
  parser->setErrorHandler(errorHandler);
  
  try
  {
    std::cout<<"Parsing "<< xmlFile <<std::endl;
    parser->parse(xmlFile);
  }
  
  catch (const XMLException& toCatch)
  {
    char* message = XMLString::transcode(toCatch.getMessage());
    std::cout<<"Error during parsing! : "<< message <<std::endl;
    XMLString::release(&message);
    return -1;
  }

  catch (const DOMException& toCatch)
  {
    char* message = XMLString::transcode(toCatch.getMessage());
    std::cout<<"Error during parsing! : "<< message <<std::endl;
    XMLString::release(&message);
    return -1;
  }
  
  catch (...)
  {
    std::cout<<"Unknown exception!"<<std::endl;
    return -1;
  }

  DOMDocument* document = parser->getDocument();
  DOMElement*  root = document->getDocumentElement();

  /* Now we can create and fill the ig file */
  IgDataStorage* storage = new IgDataStorage;
  
  IgCollection& event = storage->getCollection("Event_V3");
  IgProperty EXP = event.addProperty("experiment", std::string("ATLAS"));
  IgProperty RUN   = event.addProperty("run", 0);
  IgProperty EVENT = event.addProperty("event", 0);
  IgProperty LS    = event.addProperty("ls", 0);
  IgProperty ORBIT = event.addProperty("orbit", 0);
  IgProperty BX    = event.addProperty("bx", 0);
  IgProperty TIME  = event.addProperty("time", std::string());
  IgProperty LOCALTIME = event.addProperty("localtime", std::string());
  IgProperty MC = event.addProperty("mc", int(0));

  char* runNumber = 
    XMLString::transcode(root->getAttribute(XMLString::transcode("runNumber")));
  
  char* eventNumber = 
    XMLString::transcode(root->getAttribute(XMLString::transcode("eventNumber")));

  char* dateTime = 
    XMLString::transcode(root->getAttribute(XMLString::transcode("dateTime")));
  
  IgCollectionItem e = event.create();
  e[EXP] = std::string("ATLAS");
  e[RUN] = atoi(runNumber);
  e[EVENT] = atoi(eventNumber);


  DOMNodeList* track_types = root->getElementsByTagName(XMLString::transcode("Track"));
  const XMLSize_t ntrack_types = track_types->getLength();

  /* Let's do something simple-minded for now a cache the track information like so */
    
  std::vector<double> pt;
  std::vector<int>    numPolyline;
  std::vector<double> polylineX;
  std::vector<double> polylineY;
  std::vector<double> polylineZ;
 
  for ( XMLSize_t nt = 0; nt < ntrack_types; ++nt )
  {
    DOMNode* track_type = track_types->item(nt);
    DOMNamedNodeMap* track_type_attributes = track_type->getAttributes();
    DOMNode* track_type_attribute = track_type_attributes->getNamedItem(XMLString::transcode("storeGateKey"));
    char* track_type_name = XMLString::transcode(track_type_attribute->getNodeValue());
    
    if ( ! XMLString::equals(track_type_name, "ExtendedTracks") )
      continue;
    
    //std::cout<< track_type_name <<std::endl;

    DOMNodeList* track_child_nodes = track_type->getChildNodes();

    for ( XMLSize_t ntcn = 0; ntcn < track_child_nodes->getLength(); ++ntcn )
    {
      DOMNode* track_child_node = track_child_nodes->item(ntcn);      
      
      if ( track_child_node->getNodeType() == DOMNode::ELEMENT_NODE )
      {
        DOMElement* track_child_element = dynamic_cast<DOMElement*>(track_child_node);
        const char* node_name = XMLString::transcode(track_child_element->getNodeName());

        if ( XMLString::equals(node_name, "pt") )
        {
          //std::cout<< node_name <<": "<<std::endl;
          const XMLCh* text_content = track_child_element->getTextContent();
          //std::cout<< XMLString::transcode(text_content) <<std::endl;

          XMLStringTokenizer tokens(text_content);
          //std::cout<< tokens.countTokens() <<std::endl;

          XMLCh* token;
          int n = 0;
          while ( token = tokens.nextToken() )
          {
            pt.push_back(atof(XMLString::transcode(token)));
          }
        }
      
        if ( XMLString::equals(node_name, "numPolyline") )
        {
          //std::cout<< node_name <<": "<<std::endl;
          const XMLCh* text_content = track_child_element->getTextContent();
          //std::cout<< XMLString::transcode(text_content) <<std::endl;
          
          XMLStringTokenizer tokens(text_content);
          //std::cout<< tokens.countTokens() <<std::endl;

          XMLCh* token;
          int n = 0;
          while ( token = tokens.nextToken() )
          {
            numPolyline.push_back(atoi(XMLString::transcode(token)));
          }
        }
        
        if ( XMLString::equals(node_name, "polylineX") )
        {
          //std::cout<< node_name <<": "<<std::endl;
          const XMLCh* text_content = track_child_element->getTextContent();
          //std::cout<< XMLString::transcode(text_content) <<std::endl;

          XMLStringTokenizer tokens(text_content);
          //std::cout<< tokens.countTokens() <<std::endl;
          
          XMLCh* token;
          int n = 0;
          while ( token = tokens.nextToken() )
          {
            polylineX.push_back(atof(XMLString::transcode(token))/100.0);
          }
        }

        if ( XMLString::equals(node_name, "polylineY") )
        {
          //std::cout<< node_name <<": "<<std::endl;
          const XMLCh* text_content = track_child_element->getTextContent();
          //std::cout<< XMLString::transcode(text_content) <<std::endl;

          XMLStringTokenizer tokens(text_content);
          //std::cout<< tokens.countTokens() <<std::endl;
          
          XMLCh* token;
          int n = 0;
          while ( token = tokens.nextToken() )
          {
            polylineY.push_back(atof(XMLString::transcode(token))/100.0);
          }
        }

        if ( XMLString::equals(node_name, "polylineZ") )
        {
          //std::cout<< node_name <<": "<<std::endl;
          const XMLCh* text_content = track_child_element->getTextContent();
          //std::cout<< XMLString::transcode(text_content) <<std::endl;

          XMLStringTokenizer tokens(text_content);
          //std::cout<< tokens.countTokens() <<std::endl;
        
          XMLCh* token;
          int n = 0;
          while ( token = tokens.nextToken() )
          {
            polylineZ.push_back(atof(XMLString::transcode(token))/100.0);
          }
        }
      }
    }
  }

  IgCollection& tracks = storage->getCollection("Tracks_V3");
  IgProperty ID  = tracks.addProperty("id", int(0));
  IgProperty PT  = tracks.addProperty("pt", 0.0);
  
  IgCollection& hits = storage->getCollection("Hits_V1");
  IgProperty HT = hits.addProperty("t", 0.0);
  IgProperty X = hits.addProperty("x", 0.0);
  IgProperty Y = hits.addProperty("y", 0.0);
  IgProperty Z = hits.addProperty("z", 0.0);
  
  IgAssociations& trackhits = storage->getAssociations("TrackHits_V1");

  unsigned int pl = 0;
  unsigned int ple = 0;

  assert(pt.size() == numPolyline.size());
  assert(polylineX.size() == polylineY.size());
  assert(polylineZ.size() == polylineX.size());

  std::cout<<"There are "<< pt.size() <<" tracks"<<std::endl;

  SoSeparator* sep = new SoSeparator;

  for ( size_t i = 0, ie = pt.size(); i != ie; ++i )
  {
    SoVertexProperty* properties = new SoVertexProperty;
    int n = 0;
    
    ple += numPolyline[i];
    
    double x,y,z;

    for ( ; pl != ple; ++pl )
    {   
      x = polylineX[pl];
      y = polylineY[pl];
      z = polylineZ[pl];

      SbVec3f pos(x,y,z);
      properties->vertex.set1Value(n,pos);
      n++;
    }

    std::cout<<"Making line set"<<std::endl;
    SoLineSet* line = new SoLineSet;
    line->vertexProperty = properties;
    sep->addChild(line);
    sep->ref();

    IgCollectionItem t = tracks.create();
    t[PT] = pt[i];
   
    D0 = 0.0;
    D  = 0.0;
    T0 = 0.0;
    T  = 0.0;
 
    SoCallbackAction cba;
    cba.addLineSegmentCallback(SoShape::getClassTypeId(),
                               getHits, NULL);
    
    std::cout<<"Iterating over hits"<<std::endl;
    for ( std::vector<IgV4d>::iterator hi = sHits.begin(), hiEnd = sHits.end();
          hi != hiEnd; ++hi )
    {
      IgCollectionItem h = hits.create();
      
      h[HT] = (*hi)[0];
      h[X] = (*hi)[1];
      h[Y] = (*hi)[2];
      h[Z] = (*hi)[3];

      trackhits.associate(t,h);      
    }

    sHits.clear();

    cba.apply(sep);
    //sep->unref();
  }
  
  /*
  for ( size_t i = 0, ie = pt.size(); i != ie; ++i )
  {
    D0 = 0.0;   
    D  = 0.0;
    T0 = 0.0;
    T  = 0.0;

    IgCollectionItem t = tracks.create();
    t[PT] = pt[i];

    //std::cout<<"Track "<< i <<": pt = "<< pt[i] <<"  nhits = "<< numPolyline[i] <<std::endl;

    ple += numPolyline[i];

    double x,y,z;

    for ( ; pl != ple; ++pl )
    {   
      x = polylineX[pl];
      y = polylineY[pl];
      z = polylineZ[pl];

      D = sqrt(x*x+y*y+z*z);
      T = (D-D0)/C + T0;
      D0 = D;
      T0 = T;

      IgCollectionItem h = hits.create();
      h[HT] = T;
      h[X] = x;
      h[Y] = y;
      h[Z] = z;
   

      trackhits.associate(t, h);
    }
  }
  */
  std::cout<<"Done"<<std::endl;
  XMLPlatformUtils::Terminate();

  //std::cerr << *storage << std::endl;
  fout<< *storage <<std::endl;
  delete storage;

  return 0;
}
