package com.infinity360.remote

import android.content.Context
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.os.LocaleListCompat
import com.google.android.material.bottomnavigation.BottomNavigationView
import com.google.android.material.navigation.NavigationBarView

/** SDK/Jetpack navigation and locale bridge shared with the existing Java controller. */
object RemoteNavigation {
    private val ids = intArrayOf(R.id.nav_control, R.id.nav_catalog, R.id.nav_system, R.id.nav_connection, R.id.nav_store)
    @JvmStatic fun create(context: Context): BottomNavigationView = BottomNavigationView(context).apply {
        inflateMenu(R.menu.remote_navigation)
        labelVisibilityMode = NavigationBarView.LABEL_VISIBILITY_LABELED
    }
    @JvmStatic fun index(id: Int): Int = ids.indexOf(id)
    @JvmStatic fun select(view: BottomNavigationView?, index: Int) {
        if (view != null && index in ids.indices && view.selectedItemId != ids[index]) view.selectedItemId = ids[index]
    }
    @JvmStatic fun locale(choice: Int) {
        val language = listOf("", "pt-BR", "en", "es").getOrNull(choice) ?: return
        AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(language))
    }
}
