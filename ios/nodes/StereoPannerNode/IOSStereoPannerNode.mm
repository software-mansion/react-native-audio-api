
#include <IOSStereoPannerNode.h>

namespace audioapi {

IOSStereoPannerNode::IOSStereoPannerNode(StereoPannerNode *stereoPanner)
{
  audioNode_ = panner_ = stereoPanner;
}

IOSStereoPannerNode::~IOSStereoPannerNode()
{
  [panner_ cleanup];
  audioNode_ = panner_ = nil;
}

std::shared_ptr<IOSAudioParam> IOSStereoPannerNode::getPanParam()
{
  std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
  param->audioParam = panner_.panParam;
  return param;
}
} // namespace audioapi
