def clean_data(filename: str):
    with open(filename, 'r') as fp:
        lines = fp.readlines()
    title = lines[0][:-1]
    lines = lines[1:]
    lines = [lines[i] for i in range(0, len(lines), 2)]
    lines = [line[:-3] for line in lines]
    with open(filename, 'w') as fp:
        print(title, file=fp, end='\n')
        for line in lines:
            print(line, file=fp, end='\n')

