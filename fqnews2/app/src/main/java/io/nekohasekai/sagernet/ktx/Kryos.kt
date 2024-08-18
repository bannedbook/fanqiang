package io.nekohasekai.sagernet.ktx

import android.os.Parcel
import android.os.Parcelable
import com.esotericsoftware.kryo.io.ByteBufferInput
import com.esotericsoftware.kryo.io.ByteBufferOutput
import java.io.InputStream
import java.io.OutputStream


fun InputStream.byteBuffer() = ByteBufferInput(this)
fun OutputStream.byteBuffer() = ByteBufferOutput(this)

fun ByteBufferInput.readStringList(): List<String> {
    return mutableListOf<String>().apply {
        repeat(readInt()) {
            add(readString())
        }
    }
}

fun ByteBufferInput.readStringSet(): Set<String> {
    return linkedSetOf<String>().apply {
        repeat(readInt()) {
            add(readString())
        }
    }
}


fun ByteBufferOutput.writeStringList(list: List<String>) {
    writeInt(list.size)
    for (str in list) writeString(str)
}

fun ByteBufferOutput.writeStringList(list: Set<String>) {
    writeInt(list.size)
    for (str in list) writeString(str)
}

fun Parcelable.marshall(): ByteArray {
    val parcel = Parcel.obtain()
    writeToParcel(parcel, 0)
    val bytes = parcel.marshall()
    parcel.recycle()
    return bytes
}

fun ByteArray.unmarshall(): Parcel {
    val parcel = Parcel.obtain()
    parcel.unmarshall(this, 0, size)
    parcel.setDataPosition(0) // This is extremely important!
    return parcel
}

fun <T> ByteArray.unmarshall(constructor: (Parcel) -> T): T {
    val parcel = unmarshall()
    val result = constructor(parcel)
    parcel.recycle()
    return result
}