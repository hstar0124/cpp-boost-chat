#pragma once
#include "Common.h"
#include "MyMessage.pb.h"

class TcpSession;

struct OwnedMessage
{
	std::shared_ptr<TcpSession> remote = nullptr;
	std::shared_ptr<myChatMessage::ChatMessage> chatMessage;
};