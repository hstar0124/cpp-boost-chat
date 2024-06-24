using ApiServer.Model.Entity;
using Microsoft.EntityFrameworkCore;

namespace ApiServer.Repository.Context
{
    public class HStarContext : DbContext
    {
        public DbSet<UserEntity> User {  get; set; }

        protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
        {
            string connectionString = "server=127.0.0.1; port=3306; database=hstar; user=root; password=root";
            optionsBuilder.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString));
        }
    }
}
