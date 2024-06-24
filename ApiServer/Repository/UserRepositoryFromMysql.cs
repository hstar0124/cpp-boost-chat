using ApiServer.Model.Entity;
using ApiServer.Repository.Context;
using LoginApiServer.Model;
using LoginApiServer.Repository.Interface;
using LoginApiServer.Utils;
using Microsoft.EntityFrameworkCore;

namespace ApiServer.Repository
{
    public class UserRepositoryFromMysql : IUserRepository
    {
        private readonly ILogger<UserRepositoryFromMysql> _logger;
        private readonly HStarContext _context;

        public UserRepositoryFromMysql(ILogger<UserRepositoryFromMysql> logger, HStarContext hStarContext)
        {
            _logger = logger;
            _context = hStarContext;
        }

        public UserStatusCode CreateUser(UserEntity userEntity)
        {
            // 아이디가 이미 존재하는 경우
            UserEntity alreadyUser = _context.User.SingleOrDefault(x => x.UserId == userEntity.UserId);
            if (alreadyUser != null)
            {
                _logger.LogError("User creation failed for UserId {UserId}. UserId already exists.", userEntity.UserId);
                return UserStatusCode.UserIdAlreadyExists;
            }

            try
            {
                _context.User.Add(userEntity);
                _context.SaveChanges();

                _logger.LogInformation("User with UserId {UserId} created successfully.", userEntity.UserId);
                return UserStatusCode.Success;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "User creation failed for UserId {UserId}.", userEntity.UserId);
                return UserStatusCode.Failure;
            }
        }

        public UserStatusCode DeleteUser(UserEntity userEntity)
        {
            try
            {
                _context.User.Update(userEntity);
                _context.SaveChanges();

                _logger.LogInformation("User with UserId {UserId} deleted successfully.", userEntity.UserId);
                return UserStatusCode.Success;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "User delete failed for UserId {UserId}.", userEntity.UserId);
                return UserStatusCode.Failure;
            }
        }

        public (UserStatusCode, UserEntity) GetUserFromUserid(string id)
        {
            // 단순한 Read 작업은 Track을 끔으로 성능 향상
            UserEntity storedUser = _context.User.AsNoTracking().SingleOrDefault(x => x.UserId == id);

            // 유저가 존재하지 않은 경우
            if (storedUser == null)
            {
                _logger.LogError("User not exists :: {UserId}", id);
                return (UserStatusCode.UserNotExists, null);
            }

            // 삭제된 유저인 경우
            if (storedUser.IsAlive == "N")
            {
                _logger.LogError("User not exists :: {UserId}", id);
                return (UserStatusCode.UserNotExists, null);
            }

            _logger.LogInformation("User retrieval successfully :: {UserId}", id);
            return (UserStatusCode.Success, storedUser);
        }

        public (UserStatusCode, UserEntity) ValidateUserCredentials(UserEntity userEntity)
        {
            // 단순한 Read 작업은 Track을 끔으로 성능 향상
            UserEntity storedUser = _context.User.AsNoTracking().SingleOrDefault(x => x.UserId == userEntity.UserId);
            
            // 유저가 존재하지 않은 경우
            if (storedUser == null)
            {
                _logger.LogError("User not exists :: {UserId}", userEntity.UserId);
                return (UserStatusCode.UserNotExists, null);
            }

            // 삭제된 유저인 경우
            if(storedUser.IsAlive == "N")
            {
                _logger.LogError("User not exists :: {UserId}", userEntity.UserId);
                return (UserStatusCode.UserNotExists, null);
            }

            // 비밀번호가 틀린 경우
            if (!PasswordHelper.VerifyPassword(storedUser.Password, userEntity.Password))
            {
                _logger.LogError("Incorrect password for user :: {UserId}", userEntity.UserId);
                return (UserStatusCode.DifferentPassword, null);
            }


            _logger.LogInformation("User login successfully :: {UserId}", userEntity.UserId);
            return (UserStatusCode.Success, storedUser);
        }

        public UserStatusCode UpdateUser(UserEntity userEntity)
        {
            try
            {
                _context.User.Update(userEntity);
                _context.SaveChanges();

                _logger.LogInformation("User with UserId {UserId} updated successfully.", userEntity.UserId);
                return UserStatusCode.Success;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "User update failed for UserId {UserId}.", userEntity.UserId);
                return UserStatusCode.Failure;
            }
        }
    }
}
