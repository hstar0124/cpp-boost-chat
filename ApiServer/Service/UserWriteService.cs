using ApiServer.Model.Entity;
using Google.Protobuf.WellKnownTypes;
using LoginApiServer.Model;
using LoginApiServer.Repository.Interface;
using LoginApiServer.Service.Interface;
using LoginApiServer.Utils;
using System.Transactions;

namespace LoginApiServer.Service
{
    public class UserWriteService : IUserWriteService
    {
        private readonly ILogger<UserWriteService> _logger;
        private readonly IUserRepository _userRepository;
        private readonly ICacheRepository _cacheRepository;

        public UserWriteService (ILogger<UserWriteService> logger, IUserRepository userRepository, ICacheRepository cacheRepository)
        {
            _logger = logger;
            _userRepository = userRepository;
            _cacheRepository = cacheRepository;
        }

        public UserResponse LoginUser(LoginRequest request)
        {
            var status = UserStatusCode.Success;            

            UserEntity user = new UserEntity
            {
                UserId = request.UserId,
                Password = request.Password
            };

            try
            {
                (status, UserEntity storedUser) = _userRepository.ValidateUserCredentials(user);

                // 로그인 실패시 Fast Return
                if (status != UserStatusCode.Success)
                {
                    return new UserResponse
                    {
                        Status = status,
                        Message = "User login failed"
                    };
                }

                // 세션ID 생성
                string sessionId = Guid.NewGuid().ToString().Replace("-", "");

                // Redis에 세션 ID 저장
                status = _cacheRepository.CreateSession(sessionId, storedUser.Id);
                if (status != UserStatusCode.Success)
                {
                    _logger.LogError("An error occurred while login the User for UserId {UserId}.", request.UserId);
                    return new UserResponse
                    {
                        Status = status,
                        Message = "An error occurred while login the User"
                    };
                }

                // Chat Sever IP, Port, SessionID 를 담아서 리턴
                var loginSuccessResponse = new LoginResponse
                {
                    ServerIp = "127.0.0.1",
                    ServerPort = "4242",
                    SessionId = sessionId
                };

                return new UserResponse
                {
                    Status = status,
                    Message = (status == UserStatusCode.Success) ? "User Login successfully" : "User Login failed",
                    Content = Any.Pack(loginSuccessResponse)
                };

            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "An error occurred while Login the User for UserId {UserId}.", request.UserId);
                return new UserResponse
                {
                    Status = status,
                    Message = $"An error occurred while Login the User: {ex.Message}"
                };
            }
        }

        public UserResponse CreateUser(CreateUserRequest request)
        {
            var status = UserStatusCode.Success;

            UserEntity user = new UserEntity
            {
                UserId = request.UserId,
                Password = PasswordHelper.HashPassword(request.Password),
                Username = request.Username,
                Email = request.Email,
                IsAlive = "Y"
            };


            try
            {
                status = _userRepository.CreateUser(user);

                return new UserResponse
                {
                    Status = status,
                    Message = (status == UserStatusCode.Success) ? "User created successfully" : "User creation failed",
                    Content = Any.Pack(request)
                };
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "An error occurred while creating the User for UserId {UserId}.", request.UserId);
                return new UserResponse
                {
                    Status = status,
                    Message = $"An error occurred while creating the User: {ex.Message}"
                };
            }
        }

        public UserResponse UpdateUser(UpdateUserRequest request)
        {
            var status = UserStatusCode.Success;

            UserEntity user = new UserEntity
            {
                UserId = request.UserId,
                Password = request.Password
            };

            using (var scope = new TransactionScope())
            {
                try
                {
                    (status, UserEntity storedUser) = _userRepository.ValidateUserCredentials(user);
                    
                    // 유저가 존재하지 않거나, 비밀번호가 틀린경우
                    if (status != UserStatusCode.Success)
                    {
                        _logger.LogError("An error occurred while updating the User for UserId {UserId}.", request.UserId);
                        return new UserResponse
                        {
                            Status = status,
                            Message = "An error occurred while updating the User"
                        };
                    }

                    // 업데이트할 필드만 업데이트
                    if (!string.IsNullOrEmpty(request.ToBePassword))
                    {
                        storedUser.Password = PasswordHelper.HashPassword(request.ToBePassword);                        
                    }
                    if (!string.IsNullOrEmpty(request.ToBeUsername))
                    {
                        storedUser.Username = request.ToBeUsername;
                    }
                    if (!string.IsNullOrEmpty(request.ToBeEmail))
                    {
                        storedUser.Email = request.ToBeEmail;
                    }

                    // 사용자 정보를 업데이트
                    status = _userRepository.UpdateUser(storedUser);
                    if (status != UserStatusCode.Success)
                    {
                        _logger.LogError("An error occurred while updating the User for UserId {UserId}.", request.UserId);
                        return new UserResponse
                        {
                            Status = status,
                            Message = "An error occurred while updating the User"
                        };
                    }

                    // 트랜잭션 범위 완료
                    scope.Complete();
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "An exception occurred while updating the User for UserId {UserId}.", request.UserId);
                    status = UserStatusCode.Failure;
                    return new UserResponse
                    {
                        Status = status,
                        Message = "An exception occurred while updating the User"
                    };
                }
            }

            return new UserResponse
            {
                Status = status,
                Message = (status == UserStatusCode.Success) ? "User updated successfully" : "User update failed"
            };
        }


        public UserResponse DeleteUser(DeleteUserRequest request)
        {
            var status = UserStatusCode.Success;

            UserEntity user = new UserEntity
            {
                UserId = request.UserId,
                Password = request.Password
            };

            using (var scope = new TransactionScope())
            {
                try
                {
                    (status, UserEntity storedUser) = _userRepository.ValidateUserCredentials(user);

                    if (status != UserStatusCode.Success)
                    {
                        _logger.LogError("An error occurred while deleting the User for UserId {UserId}.", request.UserId);
                        return new UserResponse
                        {
                            Status = status,
                            Message = "An error occurred while deleting the User"
                        };
                    }

                    // 소프트 삭제
                    storedUser.IsAlive = "N";

                    status = _userRepository.DeleteUser(storedUser);
                    if (status != UserStatusCode.Success)
                    {
                        _logger.LogError("An error occurred while deleting the User for UserId {UserId}.", request.UserId);
                        return new UserResponse
                        {
                            Status = status,
                            Message = "An error occurred while deleting the User"
                        };
                    }


                    // 트랜잭션 범위 완료
                    scope.Complete();
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "An exception occurred while deleting the User for UserId {UserId}.", request.UserId);
                    status = UserStatusCode.Failure;
                    return new UserResponse
                    {
                        Status = status,
                        Message = "An exception occurred while deleting the User"
                    };
                }
            }

            return new UserResponse
            {
                Status = status,
                Message = (status == UserStatusCode.Success) ? "User deleted successfully" : "User deletion failed"
            };
        }

        
    }
}
