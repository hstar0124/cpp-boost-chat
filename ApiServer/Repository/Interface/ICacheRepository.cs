using LoginApiServer.Model;

namespace LoginApiServer.Repository.Interface
{
    public interface ICacheRepository
    {
        Task<UserStatusCode> CreateSession(string sessionId, long id);
    }
}
