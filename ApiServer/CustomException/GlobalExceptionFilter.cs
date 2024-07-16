using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.Filters;
using ApiServer.CustomException;
using Google.Protobuf; 

namespace ApiServer.Filters
{
    public class GlobalExceptionFilter : IExceptionFilter
    {
        private readonly ILogger<GlobalExceptionFilter> _logger;

        public GlobalExceptionFilter(ILogger<GlobalExceptionFilter> logger)
        {
            _logger = logger;
        }

        public void OnException(ExceptionContext context)
        {
            var exception = context.Exception;
            var exceptionType = exception.GetType();

            if (exceptionType == typeof(UserNotFoundException))
            {
                HandleUserNotFoundException(context, exception);
            }
            else if (exceptionType == typeof(UserValidationException))
            {
                HandleUserValidationException(context, exception);
            }
            else
            {
                HandleUnknownException(context, exception);
            }

            context.ExceptionHandled = true;
        }

        private void HandleUserNotFoundException(ExceptionContext context, Exception exception)
        {
            var response = new UserErrorResponse
            {
                Message = exception.Message
            };

            SetResponse(context, response, 404);
        }

        private void HandleUserValidationException(ExceptionContext context, Exception exception)
        {
            var response = new UserErrorResponse
            {
                Message = exception.Message
            };

            SetResponse(context, response, 400);
        }

        private void HandleUnknownException(ExceptionContext context, Exception exception)
        {
            var response = new UserErrorResponse
            {
                Message = "An unexpected error occurred."
            };

            _logger.LogError(exception, exception.Message);
            SetResponse(context, response, 500);
        }

        private void SetResponse(ExceptionContext context, IMessage response, int statusCode)
        {
            context.HttpContext.Response.StatusCode = statusCode;
            context.Result = new ObjectResult(response)
            {
                StatusCode = statusCode,
                DeclaredType = typeof(IMessage) // 사용하는 프로토콜 버퍼 메시지 타입으로 변경
            };
        }
    }
}
