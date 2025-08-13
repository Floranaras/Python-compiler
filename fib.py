def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

print("Fibonacci sequence:")
print(fibonacci(5))
print(fibonacci(20))
