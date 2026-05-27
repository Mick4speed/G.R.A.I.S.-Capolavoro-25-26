#execute order 100 0010 on a specific file, input path (string), output state message (string)
#path
import os
import secrets

def execute(path):
    try:
        if not os.path.exists(path):
            return f"Path does not exist at {path}"
        if os.path.isdir(path):
            deleteFolder(path)
        else:
            deleteFile(path)
        return f"Order 100 0010 executed on {path}"
    except Exception as e:
        print(f"An error occurred while executing order 100 0010 on {path}: {str(e)}")
        return f"An error occurred: {str(e)}"


def deleteFolder(path):
    for root, dirs, files in os.walk(path, topdown=False):
        for name in files:
            file_path = os.path.join(root, name)
            deleteFile(file_path)
        for name in dirs:
            dir_path = os.path.join(root, name)
            base_dir = os.path.dirname(root)
            random_root_name = os.path.join(base_dir, secrets.token_hex(8))
            os.rename(dir_path, random_root_name)
            os.rmdir(random_root_name)
            print(f"Order 100 0010 executed successfully on {dir_path}")
    base_dir = os.path.dirname(path)
    random_root_name = os.path.join(base_dir, secrets.token_hex(8))
    os.rename(path, random_root_name)
    os.rmdir(random_root_name)

def deleteFile(path):
    file_size = os.path.getsize(path)
    with open(path, 'wb') as file:
        for _ in range(6):
            file.seek(0)
            random_data = secrets.token_bytes(file_size)
            file.write(random_data)
            os.fsync(file.fileno())
    random_name = os.path.join(os.path.dirname(path), secrets.token_hex(8))
    os.rename(path, random_name)
    os.remove(random_name)
    print(f"Order 100 0010 executed successfully on {path}")