void error_fatal(const char *msg, VkResult err)
{
        fprintf(stderr, "Error: %s (Result=%s)\n", msg, MAGIC_ENUM_NAME(err));
        exit(EXIT_FAILURE);
}