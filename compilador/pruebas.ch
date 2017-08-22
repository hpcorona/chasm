; Este programa es para intentar probar
; que el compilador CHC este funcionando
; a la perfeccion.
; Es un tipico ABC con vectores.
; La maquina virtual CHVM no soporta guardar
; a un archivo, por lo tanto todas las
; operaciones seran solamente realizadas en
; memoria.
; (c) 2007 Hilario Perez Corona
char[10, 50] nombre, direccion
char[10, 20] telefono
integer[10] edad
double[10] salario
char[50] tnombre, tdireccion
char[20] ttelefono
integer tedad
double tsalario
integer total, opcion, i, j, orden, cambiar

start
	;Datos de Prueba :)
	nombre[0] = "Z"
	direccion[0] = "A"
	telefono[0] = "Q"
	edad[0] = 3
	salario[0] = 1
	nombre[1] = "A"
	direccion[1] = "Z"
	telefono[1] = "W"
	edad[1] = 1
	salario[1] = 2
	total = 2

	opcion = 1
	while (opcion != 0)
		println("MENU PRINCIPAL")
		println("1. altas")
		println("2. bajas")
		println("3. modificaciones")
		println("4. listado")
		println("5. ordenar datos")
		println("")
		println("0. salir")
		print("ELIJA UNA OPCION: ")
		read(opcion)

		if (opcion == 1)
			if (total < 10)
				println("ALTA DE PERSONA")
				print("nombre: ")
				read(nombre[total])
				print("direccion: ")
				read(direccion[total])
				print("telefono: ")
				read(telefono[total])
				print("edad: ")
				read(edad[total])
				print("salario: ")
				read(salario[total])

				total = total + 1
				println("la persona ha sido dada de alta!")
			else
				println("error: el vector se encuentra lleno")
			endif
		else if (opcion == 2)
			if (total > 0)
				print("ingrese la posicion a eliminar: ")
				read(i)

				if (i < 1 || i > total)
					println("error: posicion invalida")
				else
					i = i - 1
					for (j = i, j < total - 1, j = j + 1)
						nombre[j] = nombre[j + 1]
						direccion[j] = direccion[j + 1]
						telefono[j] = telefono[j + 1]
						edad[j] = edad[j + 1]
						salario[j] = salario[j + 1]
					endfor
					total = total - 1
					println("la persona fue borrada")
				endif
			else
				println("error: el vector se encuentra vacio")
			endif
		else if (opcion == 3)
			if (total > 0)
				print("ingrese la posicion a modificar: ")
				read(i)

				if (i < 1 || i > total)
					println("error: posicion invalida")
				else
					i = i - 1
					print("nombre: ")
					read(nombre[i])
					print("direccion: ")
					read(direccion[i])
					print("telefono: ")
					read(telefono[i])
					print("edad: ")
					read(edad[i])
					print("salario: ")
					read(salario[i])

					println("la persona fue modificada")
				endif
			else
				println("error: el vector se encuentra vacio")
			endif
		else if (opcion == 4)
			if (total > 0)
				println("POS\tNOMBRE\tDIRECCION\tTELEFONO\tEDAD\tSALARIO")
				for (i = 0, i < total, i = i + 1)
					print(i + 1)
					print("\t")
					print(nombre[i])
					print("\t")
					print(direccion[i])
					print("\t")
					print(telefono[i])
					print("\t")
					print(edad[i])
					print("\t")
					println(salario[i])
				endfor
				println("TOTAL: " & total & " personas")
			else
				println("error: el vector se encuentra vacio")
			endif
		else if (opcion == 5)
			if (total > 0)
				println("Ordenar por: 1. Nombre  2. Direccion  3. Telefono  4. Edad  5. Salario")
				print("ORDENAR POR: ")
				read(orden)

				if (orden >= 1 && orden <= 5)
					for (i = 0, i < total - 1, i = i + 1)
						for (j = i + 1, j < total, j = j + 1)
							cambiar = 0
							if (orden == 1)
								if (nombre[i] > nombre[j])
									cambiar = 1
								endif
							else if (orden == 2)
								if (direccion[i] > direccion[j])
									cambiar = 1
								endif
							else if (orden == 3)
								if (telefono[i] > telefono[j])
									cambiar = 1
								endif
							else if (orden == 4)
								if (edad[i] > edad[j])
									cambiar = 1
								endif
							else if (orden == 5)
								if (salario[i] > salario[j])
									cambiar = 1
								endif
							endif endif endif endif endif
							if (cambiar == 1)
								tnombre = nombre[i]
								tdireccion = direccion[i]
								ttelefono = telefono[i]
								tedad = edad[i]
								tsalario = salario[i]

								nombre[i] = nombre[j]
								direccion[i] = direccion[j]
								telefono[i] = telefono[j]
								edad[i] = edad[j]
								salario[i] = salario[j]

								nombre[j] = tnombre
								direccion[j] = tdireccion
								telefono[j] = ttelefono
								edad[j] = tedad
								salario[j] = tsalario
							endif
						endfor
					endfor

					println("los datos han sido ordenados")
				else
					println("error: orden invalido")
				endif
			else
				println("error: el vector se encuentra vacio")
			endif
		endif endif endif endif endif
	endwhile

	println("adiosin :)")
end
