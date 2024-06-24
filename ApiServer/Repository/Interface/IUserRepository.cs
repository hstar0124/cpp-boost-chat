
using ApiServer.Model.Entity;
using LoginApiServer.Model;

namespace LoginApiServer.Repository.Interface
{
    public interface IUserRepository
    {
        UserStatusCode CreateUser(UserEntity userEntity);
        UserStatusCode UpdateUser(UserEntity userEntity);
        UserStatusCode DeleteUser(UserEntity userEntity);
        (UserStatusCode, UserEntity) GetUserFromUserid(string id);
        (UserStatusCode, UserEntity) ValidateUserCredentials(UserEntity userEntity);
    }
}
