using System;

namespace ApiServer.CustomException
{
    public class UserNotFoundException : Exception
    {
        public UserNotFoundException() : base() { }

        public UserNotFoundException(string message) : base(message) { }

        public UserNotFoundException(string message, Exception innerException) : base(message, innerException) { }
    }

    public class UserValidationException : Exception
    {
        public UserValidationException() : base() { }

        public UserValidationException(string message) : base(message) { }

        public UserValidationException(string message, Exception innerException) : base(message, innerException) { }
    }

}
