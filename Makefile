db: Database.c
	gcc Database.c -o db

run: db
	./db mydb.db

clean:
	rm -f db *.db