using Google.Protobuf;
using Microsoft.AspNetCore.Mvc;

namespace ApiServer.CustomException
{
    public static class ProtobufException
    {
        public static UserResponse CreateErrorResult(UserStatusCode statusCode, string message)
        {
            return new UserResponse
            {
                Status = statusCode,
                Message = message
            };
        }
    }
}
