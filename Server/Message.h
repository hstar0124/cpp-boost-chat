#pragma once
#include "Common.h"
#include "Payload.pb.h"

class TcpSession;

struct OwnedMessage
{
	std::shared_ptr<TcpSession> remote = nullptr;
	std::shared_ptr<myChatMessage::ChatMessage> payload;
};